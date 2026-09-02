#pragma once
#include <algorithm>
#include <ios>
#include <bit>
#include <cstdint>
#include <cstddef>
#include <fstream>
#include <limits>
#include <sstream>
#include <format>
#include <openssl/sha.h>
#include <zlib.h>
#include <chrono>
#include "logger.h"
#include "arguments.h"
#include "sync.h"

namespace shi
{
    using Byte = uint8_t;
    namespace fs = std::filesystem;

    constexpr std::string_view SHI_DIR = ".shi";
    constexpr std::string_view SHI_OBJECTS_DIR = ".shi/objects";
    constexpr std::string_view SHI_TREE_PATH = ".shi/_shi_tree.txt";
    constexpr std::string_view SHI_STAGING_PATH = ".shi/index/staging.bin";
    std::vector<Byte> TREE_TYPE = {'t', 'r', 'e', 'e'};
    std::vector<Byte> BLOB_TYPE = {'b', 'l', 'o', 'b'};
    constexpr char PATH_DELIMITER = '/';
    constexpr char ARG_DELIMITER = ' ';
    constexpr bool is_big_endian = std::endian::native == std::endian::big;  // C++20

    using namespace args; 

    struct File
    {
        fs::path file_path;
        std::uintmax_t size{};
        Byte mod{};
        std::vector<Byte> raw_content;
    };

    struct Shi
    {
        fs::path shi_path;
        File src_file;
        std::vector<Byte> blob_raw;
        std::vector<Byte> blob_hash;
        std::vector<Byte> type;
    };

    struct Stage
    {
        std::uint16_t mod{};
        std::int64_t mtime{};
        std::vector<Byte> hash;
        std::vector<Byte> path;
    };

    struct TreeShi
    {
        std::vector<Shi*> shis;
        std::vector<Byte> branch;
    };

    struct HashRes
    {
        std::string dir;
        std::vector<Byte> file_hash;
        std::vector<Byte> full_hash;
    };

    inline fs::path bytesToPath(const std::vector<Byte>& bytes)
    {
        return fs::path(reinterpret_cast<const char*>(bytes.data()));
    }

    inline std::vector<Byte> pathToBytes(const fs::path& path)
    {
        const auto& str = path.string();
        return std::vector<Byte>(str.begin(), str.end());
    }

    std::string inline hashToString(const Byte* hash, size_t length)
    {
        std::ostringstream ss;
        for(size_t i = 0; i < length; i++)
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        return ss.str();
    }

    std::string inline hashToString(const std::vector<Byte>& hash)
    {
        std::ostringstream ss;
        for(size_t i = 0; i < hash.size(); i++)
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        return ss.str();
    }

    inline HashRes sha256(const std::vector<Byte>& to_hash) 
    {
        unsigned char shaed[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(to_hash.data()), to_hash.size(), shaed);
        
        std::string dir_hash = hashToString(shaed, 2);

        return HashRes{
            .dir = dir_hash,
            .file_hash = {shaed + 2, shaed + SHA256_DIGEST_LENGTH},
            .full_hash = {shaed, shaed + SHA256_DIGEST_LENGTH}
        };
    }

    inline std::vector<Byte> readFile(std::istream& is)
    {   
        is.seekg(0, std::ios::end);
        const std::streampos end = is.tellg();
        if(end < 0 || end > std::numeric_limits<std::streamsize>::max()) return {};

        const std::streamsize size = static_cast<std::streamsize>(end);
        is.seekg(0, std::ios::beg);
        std::vector<Byte> content(size);

        if(size == 0) return content;

        is.read(reinterpret_cast<char*>(content.data()), size);
        if(is.gcount() != size)
        {
            logger::log(logger::level::ERROR, "Failed to read the complete file.");
            return {};
        }
        return content;
    }

    inline File getFile(const fs::path& f)
    {
        if(!fs::is_regular_file(f)) return File();

        File new_file;
        new_file.file_path = f;

        std::error_code ec;
        new_file.size = fs::file_size(f, ec);
        if(ec || new_file.size > std::numeric_limits<std::size_t>::max() || new_file.size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max()))
        {
            return File();
        }
        
        std::ifstream open_file(f, std::ios::binary);
        if (!open_file.is_open()) return File(); 

        // Pre allocate vector size directly to prevent copying loops
        const auto content_size = static_cast<std::size_t>(new_file.size);
        new_file.raw_content = readFile(open_file);

        if(new_file.raw_content.size() != content_size)
        {
            return File();
        }
        
        return new_file;
    }

    inline bool isShiDir(const fs::path& p)
    {
        return fs::exists(p / SHI_DIR) && fs::is_directory(p / SHI_DIR);
    }

    inline std::vector<Byte> compressShi(const std::vector<Byte>& data)
    {
        if(data.size() > std::numeric_limits<uLong>::max())
        {
            logger::log(logger::level::ERROR, "Input is too large for the zlib API.");
            return {};
        }

        uLong src_len = static_cast<uLong>(data.size());
        uLongf dest_len = compressBound(src_len);
        std::vector<Byte> dest(dest_len);

        // Keep a non-null source pointer even for an empty input vector.
        const Byte empty = 0;
        const Byte* source = data.empty() ? &empty : data.data();
        int res = compress(dest.data(), &dest_len, source, src_len);
        if(res != Z_OK)
        {
            logger::log(
                logger::level::ERROR, 
                logger::msgFormat("Compression failed with error code: " + std::to_string(res), __FUNCTION__, __FILE__, __LINE__)
            );
            return {};
        }

        dest.resize(dest_len);
        logger::log(logger::level::INFO, "Successfully compressed data from " + std::to_string(data.size()) + " bytes to " + std::to_string(dest_len) + " bytes.");
        return dest;
    }

    /*
    * Name: createBlobFile
    * Description: Creates a blob file for the given Shi object
    * Parameters:
    *   blob_obj: The Shi object for which to create a blob file
    * Returns: True if the blob file was created successfully, false otherwise
    */
    inline bool createBlobFile(const Shi& blob_obj)
    {
        const fs::path& shi_path = blob_obj.shi_path;
        const fs::path parent_dir = shi_path.parent_path();
        const fs::path objects_root = parent_dir.parent_path();

        // 1. Verify that the base objects repository exists
        if(!fs::exists(objects_root))
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Base objects directory does not exist: " + objects_root.string(), __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        // 2. Ensure the 2-character prefix parent directory exists
        if(!fs::exists(parent_dir))
        {
            std::error_code ec;
            fs::create_directories(parent_dir, ec);
            if(ec)
            {
                logger::log(logger::level::ERROR, logger::msgFormat("Failed to create directory " + parent_dir.string() + ": " + ec.message(), __FUNCTION__, __FILE__, __LINE__));
                return false;
            }
        }

        // 3. Skip if blob is already stored
        if(fs::exists(shi_path))
        {
            logger::log(logger::level::WARN, logger::msgFormat("Blob file already exists at: " + shi_path.string(), __FUNCTION__, __FILE__, __LINE__));
            return true;
        }

        // 4. Compress content
        const auto& raw_content = blob_obj.src_file.raw_content;
        std::vector<Byte> compressed_data = compressShi(raw_content);
        
        // Even an empty source file produces a non-empty zlib stream.
        if(compressed_data.empty())
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Aborting blob creation due to compression error on: " + shi_path.string(), __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        // 5. Write to file
        std::ofstream blob_file(shi_path, std::ios::binary);
        if(!blob_file.is_open())
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Failed to open blob file for writing: " + shi_path.string(), __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        blob_file.write(reinterpret_cast<const char*>(compressed_data.data()),
                        static_cast<std::streamsize>(compressed_data.size()));
        if(!blob_file)
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Failed while writing blob file: " + shi_path.string(), __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        blob_file.close();
        if(!blob_file)
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Failed while closing blob file: " + shi_path.string(), __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        logger::log(logger::level::INFO, "Successfully created blob file at: " + shi_path.string());
        return true;
    }

    inline bool commit(const Shi& shi, const std::string&& branch)
    {
        std::string tree_path = branch + std::string(SHI_TREE_PATH);
        std::fstream tree_file(tree_path);

        (void)shi;
        (void)branch;
        return false;
    }

    inline void readStagingFile(std::vector<Stage>& staging_data)
    {
        // Read the staging file
        std::ifstream staging_input(SHI_STAGING_PATH, std::ios::binary);
        while(staging_input)
        {
            Stage stage;
            std::uint32_t hash_size{};
            std::uint32_t path_size{};

            if(!staging_input.read(reinterpret_cast<char*>(&stage.mod), sizeof(stage.mod)) ||
               !staging_input.read(reinterpret_cast<char*>(&stage.mtime), sizeof(stage.mtime)) ||
               !staging_input.read(reinterpret_cast<char*>(&hash_size), sizeof(hash_size)) ||
               !staging_input.read(reinterpret_cast<char*>(&path_size), sizeof(path_size)))
            {
                break;
            }

            stage.hash.resize(hash_size);
            stage.path.resize(path_size);
            if(!staging_input.read(reinterpret_cast<char*>(stage.hash.data()), hash_size) ||
               !staging_input.read(reinterpret_cast<char*>(stage.path.data()), path_size))
            {
                logger::log(logger::level::ERROR, "Staging file contains an incomplete record.");
            }
            staging_data.push_back(stage);
        }

        staging_input.close();
    }

    inline bool stage(const Shi& shi)
    {
        std::vector<Stage> staging_data;

        readStagingFile(staging_data);

        // Check for duplicates and add shi data
        const auto it = std::find_if(staging_data.begin(), staging_data.end(), [&shi](const Stage& s) {
            return s.hash == shi.blob_hash;
        });

        if(it != staging_data.end())
        {
            logger::log(logger::level::INFO, "Stage already exists for: " + hashToString(shi.blob_hash));
            return true;
        }

        auto modFs = fs::status(shi.src_file.file_path);
        std::uint16_t mod = modFs.type() == fs::file_type::regular ? 0644 : 0755;
        auto mtimeFs = std::chrono::file_clock::to_sys(fs::last_write_time(shi.src_file.file_path));
        logger::log(logger::level::INFO, std::to_string(mtimeFs));
        std::int64_t mtime = static_cast<std::int64_t>(mtimeFs.time_since_epoch().count());

        staging_data.push_back(Stage{
            .mod = mod,
            .mtime = mtime,
            .hash = shi.blob_hash,
            .path = pathToBytes(shi.src_file.file_path)
        });

        // Write to staging file
        std::ofstream staging_file(SHI_STAGING_PATH, std::ios::binary | std::ios::trunc);
        if(!staging_file.is_open())
        {
            logger::log(logger::level::ERROR, "Failed to open staging file for writing.");
            return false;
        }

        for(const auto& stage : staging_data)
        {
            if(stage.hash.size() > std::numeric_limits<std::uint32_t>::max() || stage.path.size() > std::numeric_limits<std::uint32_t>::max())
            {
                logger::log(logger::level::ERROR, "Staging record is too large.");
                return false;
            }

            const auto hash_size = static_cast<std::uint32_t>(stage.hash.size());
            const auto path_size = static_cast<std::uint32_t>(stage.path.size());
            staging_file.write(reinterpret_cast<const char*>(&stage.mod), sizeof(stage.mod));
            staging_file.write(reinterpret_cast<const char*>(&stage.mtime), sizeof(stage.mtime));
            staging_file.write(reinterpret_cast<const char*>(&hash_size), sizeof(hash_size));
            staging_file.write(reinterpret_cast<const char*>(&path_size), sizeof(path_size));
            staging_file.write(reinterpret_cast<const char*>(stage.hash.data()), hash_size);
            staging_file.write(reinterpret_cast<const char*>(stage.path.data()), path_size);
        }
        if(!staging_file)
        {
            logger::log(logger::level::ERROR, "Failed while writing staging file.");
            return false;
        }
        staging_file.close();

        return true;
    }

    inline std::string catStage()
    {
        std::vector<Stage> staging_data;
        readStagingFile(staging_data);
        std::ostringstream oss;
        for(const auto& stage : staging_data)
        {
            auto mtime = stage.mtime;

            oss << stage.mod << " " << mtime << " " << hashToString(stage.hash) << " " << bytesToPath(stage.path).string() << std::endl;
        }
        return oss.str();
    }

    Shi blobbify(const fs::path& p)
    {
        Shi blob_obj;
        File f;
        
        f = getFile(p);
        logger::log(logger::level::INFO, "Creating blob for: " + f.file_path.string());

        blob_obj.src_file = f;

        blob_obj.type = fs::is_directory(p) ? TREE_TYPE : BLOB_TYPE;
        std::string header(blob_obj.type.begin(), blob_obj.type.end());
        header += std::to_string(f.size) + '\0';

        blob_obj.blob_raw.reserve(header.size() + f.raw_content.size());
        blob_obj.blob_raw.insert(blob_obj.blob_raw.end(), header.begin(), header.end());
        blob_obj.blob_raw.insert(blob_obj.blob_raw.end(), f.raw_content.begin(), f.raw_content.end());

        HashRes hash_res = sha256(blob_obj.blob_raw);
        blob_obj.blob_hash = hash_res.full_hash;
        blob_obj.shi_path = fs::path(SHI_OBJECTS_DIR) / hash_res.dir / hashToString(hash_res.file_hash);
        
        return blob_obj;
    }

    inline bool createTree(const std::string& branch)
    {
        (void)branch;
        const fs::path shi_tree = fs::path(SHI_TREE_PATH);
        std::ofstream file(shi_tree);
        if(!file.is_open())
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Failed to open tree file: " + shi_tree.string(), __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        logger::log(logger::level::INFO, "Creating tree file: " + shi_tree.string());
        file.close();
        return true;
    }

    bool init(const std::string& arg = "")
    {
        logger::log(logger::level::INFO, "Initializing .shi directory.");
        // 1. Check if argument is empty
        if(arg.empty()) 
        {
            logger::log(
                logger::level::ERROR, 
                logger::msgFormat("No arguments passed to init.", __FUNCTION__, __FILE__, __LINE__)
            );
            return false;
        }

        // 2. Resolve path using arg if intended (or use current_path if empty)
        fs::path base_path = arg;
        fs::path init_path = base_path / SHI_OBJECTS_DIR;
        fs::path staging_dir = base_path / fs::path(SHI_STAGING_PATH).parent_path();


        // 3. Create directories with error_code to avoid uncaught exceptions
        std::error_code ec;
        fs::create_directories(init_path, ec);

        if(ec || !fs::exists(init_path))
        {
            std::string err_msg = "Failed to create directory at: " + init_path.string();
            if(ec) err_msg += " (Error: " + ec.message() + ")";

            logger::log(
                logger::level::ERROR, 
                logger::msgFormat(err_msg, __FUNCTION__, __FILE__, __LINE__)
            );
            return false;
        }

        fs::create_directories(staging_dir, ec);
        if(ec)
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Failed to create staging directory: " + staging_dir.string() + " (Error: " + ec.message() + ")", __FUNCTION__, __FILE__, __LINE__));
            return false;
        }
        
        createTree("master");
        logger::log(logger::level::INFO, "Successfully initialized .shi directory at: " + init_path.string());

        return true;
    }

    bool add(std::string arg)
    {
        if(arg.size() <= 0) 
        {
            logger::log(logger::level::ERROR, logger::msgFormat("No arguments passed to add.", __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        if(arg == ".")
        {
            // Recursive search of files in parent to all children files excluding the .shi folder
        }

        // `arg` is already one command-line argument, so do not split it on
        // spaces; filenames containing spaces are valid paths.
        std::vector<fs::path> paths{fs::relative(fs::path(arg))};

        for(const auto& path : paths)
        {
            if(!fs::is_regular_file(path))
            {
                logger::log(logger::level::ERROR, logger::msgFormat("Path is not a regular file: " + path.string(), __FUNCTION__, __FILE__, __LINE__));
                return false;
            }

            Shi shi = blobbify(path);
            if(!createBlobFile(shi))
            {
                logger::log(logger::level::ERROR, logger::msgFormat("Failed to create blob file for path: " + path.string(), __FUNCTION__, __FILE__, __LINE__));
                return false;
            }
            stage(shi);
        }

        return true;
    }

    bool sync(std::string project_name, const std::vector<std::string>& args)
    {
        std::vector<fs::path> files_to_sync(args.begin(), args.end());
        if(rsync::rsync(project_name, files_to_sync))
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Failed to sync files to remote destination.", __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        logger::log(logger::level::INFO, "Successfully synced files to remote destination for project: " + project_name);
        return true;
    }
};
