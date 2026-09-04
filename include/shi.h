#pragma once
#include <algorithm>
#include <ios>
#include <bit>
#include <cstdint>
#include <cstddef>
#include <fstream>
#include <limits>
#include <openssl/sha.h>
#include <zlib.h>
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
        Byte flag{};
    };

    struct HashRes
    {
        std::string dir;
        std::vector<Byte> file_hash;
        std::vector<Byte> full_hash;
    };

    /*
     * Name: bytesToPath
     * Description: Converts a byte vector containing path text into a filesystem path
     * Parameters:
     *   bytes: The byte vector containing the path text
     * Returns: The filesystem path represented by bytes
     */
    inline fs::path bytesToPath(const std::vector<Byte>& bytes)
    {
        return fs::path(std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    }

    /*
     * Name: stringToBytes
     * Description: Converts a string into a byte vector
     * Parameters:
     *   str: The string to convert
     * Returns: The string text represented as bytes
     */
    inline std::vector<Byte> stringToBytes(const std::string& str)
    {
        return std::vector<Byte>(str.begin(), str.end());
    }

    /*
     * Name: pathToBytes
     * Description: Converts a filesystem path into a byte vector
     * Parameters:
     *   path: The filesystem path to convert
     * Returns: The path text represented as bytes
     */
    inline std::vector<Byte> pathToBytes(const fs::path& path)
    {
        const auto& str = path.string();
        return std::vector<Byte>(str.begin(), str.end());
    }

    /*
     * Name: hashToString
     * Description: Converts a byte buffer into a lowercase hexadecimal string
     * Parameters:
     *   hash: Pointer to the hash bytes
     *   length: Number of bytes in hash
     * Returns: The hexadecimal representation of the hash
     */
    std::string inline hashToString(const Byte* hash, size_t length)
    {
        std::ostringstream ss;
        for(size_t i = 0; i < length; i++)
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        return ss.str();
    }

    /*
     * Name: hashToString
     * Description: Converts a hash byte vector into a lowercase hexadecimal string
     * Parameters:
     *   hash: The hash bytes to convert
     * Returns: The hexadecimal representation of the hash
     */
    std::string inline hashToString(const std::vector<Byte>& hash)
    {
        std::ostringstream ss;
        for(size_t i = 0; i < hash.size(); i++)
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        return ss.str();
    }

    /*
     * Name: sha256
     * Description: Calculates the SHA-256 digest of a byte vector
     * Parameters:
     *   to_hash: The bytes to hash
     * Returns: The digest split into its directory, file, and full hash parts
     */
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

    /*
     * Name: readFile
     * Description: Reads all remaining bytes from a seekable input stream
     * Parameters:
     *   is: The binary input stream to read
     * Returns: The bytes read; an empty vector means an empty stream or a failed read
     */
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

    /*
     * Name: getFile
     * Description: Loads a regular file and its binary contents
     * Parameters:
     *   f: The path of the file to load
     * Returns: A File containing metadata and raw contents, or an empty File on failure
     */
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

    /*
     * Name: isShiDir
     * Description: Checks whether a path contains a .shi directory
     * Parameters:
     *   p: The directory path to inspect
     * Returns: True if p/.shi exists and is a directory, false otherwise
     */
    inline bool isShiDir(const fs::path& p)
    {
        return fs::exists(p / SHI_DIR) && fs::is_directory(p / SHI_DIR);
    }

    /*
     * Name: compressShi
     * Description: Compresses bytes using zlib
     * Parameters:
     *   data: The bytes to compress
     * Returns: The compressed bytes, or an empty vector if compression fails
     */
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

        // Verify that the base objects repository exists
        if(!fs::exists(objects_root))
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Base objects directory does not exist: " + objects_root.string(), __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        // Ensure the 2-character prefix parent directory exists
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

        // Skip if blob is already stored
        if(fs::exists(shi_path))
        {
            logger::log(logger::level::WARN, logger::msgFormat("Blob file already exists at: " + shi_path.string(), __FUNCTION__, __FILE__, __LINE__));
            return true;
        }

        // Compress content
        const auto& raw_content = blob_obj.src_file.raw_content;
        std::vector<Byte> compressed_data = compressShi(raw_content);
        
        // Even an empty source file produces a non-empty zlib stream.
        if(compressed_data.empty())
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Aborting blob creation due to compression error on: " + shi_path.string(), __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        // Write to file
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

    /*
     * Name: commit
     * Description: Commits a Shi object to a branch (not implemented)
     * Parameters:
     *   shi: The Shi object to commit
     *   branch: The branch name
     * Returns: False because commit support is not implemented
     */
    inline bool commit(const Shi& shi, const std::string&& branch)
    {
        std::string tree_path = branch + std::string(SHI_TREE_PATH);
        std::fstream tree_file(tree_path);

        (void)shi;
        (void)branch;
        return false;
    }

    /*
     * Name: readStagingFile
     * Description: Reads serialized staging records from the staging file
     * Parameters:
     *   staging_data: Output vector populated with staging records
     * Returns: None
     */
    inline void readStagingFile(std::vector<Stage>& staging_data)
    {
        staging_data.clear();

        // Read the staging file
        std::ifstream staging_input(SHI_STAGING_PATH, std::ios::binary);
        while(staging_input)
        {
            Stage stage;
            std::uint32_t hash_size{};
            std::uint32_t path_size{};
            Byte flag{};

            if(!staging_input.read(reinterpret_cast<char*>(&stage.mod), sizeof(stage.mod)) ||
               !staging_input.read(reinterpret_cast<char*>(&stage.mtime), sizeof(stage.mtime)) ||
               !staging_input.read(reinterpret_cast<char*>(&hash_size), sizeof(hash_size)) ||
               !staging_input.read(reinterpret_cast<char*>(&path_size), sizeof(path_size)) ||
               !staging_input.read(reinterpret_cast<char*>(&flag), sizeof(flag)))
            {
                break;
            }

            if(hash_size > staging_data.max_size() || path_size > staging_data.max_size())
            {
                logger::log(logger::level::ERROR, "Staging file contains an invalid record size.");
                return;
            }

            stage.hash.resize(hash_size);
            stage.path.resize(path_size);
            stage.flag = flag;
            if(!staging_input.read(reinterpret_cast<char*>(stage.hash.data()), hash_size) ||
               !staging_input.read(reinterpret_cast<char*>(stage.path.data()), path_size))
            {
                logger::log(logger::level::ERROR, "Staging file contains an incomplete record.");
                return;
            }
            staging_data.push_back(stage);
        }

        staging_input.close();
    }

    /*
     * Name: stage
     * Description: Adds a Shi object to the staging file unless it is already staged
     * Parameters:
     *   shi: The Shi object to stage
     * Returns: True if staging succeeds or the object is already staged, false otherwise
     */
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
        std::int64_t mtime = static_cast<std::int64_t>(mtimeFs.time_since_epoch().count());

        staging_data.push_back(Stage{
            .mod = mod,
            .mtime = mtime,
            .hash = shi.blob_hash,
            .path = pathToBytes(shi.src_file.file_path),
            .flag = Byte(0)
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
            staging_file.write(reinterpret_cast<const char*>(&stage.flag), sizeof(stage.flag));
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

    /*
     * Name: catStage
     * Description: Formats the current staging records for display
     * Parameters:
     *   None
     * Returns: A human-readable representation of the staging records
     */
    inline std::string catStage()
    {
        std::vector<Stage> staging_data;
        readStagingFile(staging_data);
        std::ostringstream oss;
        for(const auto& stage : staging_data)
        {
            auto mtime = stage.mtime;

            oss << stage.mod << " " << mtime << " " << static_cast<unsigned int>(stage.flag)
                << " " << hashToString(stage.hash) << " " << bytesToPath(stage.path).string() << std::endl;
        }
        return oss.str();
    }

    /*
     * Name: blobbify
     * Description: Builds a content-addressed Shi blob representation for a path
     * Parameters:
     *   p: The regular file path to convert into a blob
     * Returns: The constructed Shi object, including its hash and storage path
     */
    Shi blobbify(const fs::path& p)
    {
        Shi blob_obj;
        File f;
        
        f = getFile(p);
        logger::log(logger::level::INFO, "Creating blob for: " + f.file_path.string());

        blob_obj.src_file = f;
        blob_obj.type = BLOB_TYPE;

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

    /*
     * Name: createTree
     * Description: Creates the repository tree file
     * Parameters:
     *   branch: Branch name reserved for tree support
     * Returns: True if the tree file is created successfully, false otherwise
     */
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

    /*
     * Name: init
     * Description: Initializes the .shi object and staging directories
     * Parameters:
     *   arg: Repository base path
     * Returns: True if initialization succeeds, false otherwise
     */
    bool init()
    {
        logger::log(logger::level::INFO, "Initializing .shi directory.");

        fs::path init_path = SHI_OBJECTS_DIR;
        fs::path staging_dir =  fs::path(SHI_STAGING_PATH).parent_path();


        // Create directories with error_code to avoid uncaught exceptions
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

    /*
     * Name: add
     * Description: Creates and stages a blob for a regular file
     * Parameters:
     *   arg: Path of the file to add
     * Returns: True if the blob is stored and staged, false otherwise
     */
    bool add(std::string arg)
    {
        if(arg.size() <= 0) 
        {
            logger::log(logger::level::ERROR, logger::msgFormat("No arguments passed to add.", __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        std::vector<fs::path> paths{fs::relative(fs::path(arg))};
        if(arg == ".")
        {
            for(const auto& entry : fs::recursive_directory_iterator(fs::current_path()))
            {
                logger::log(logger::level::INFO, logger::msgFormat("Inspecting path: " + entry.path().string(), __FUNCTION__, __FILE__, __LINE__));
                std::string entry_path_str = entry.path().string();
                bool skip = [entry_path_str](std::vector<std::string>& patterns) {
                    for(const auto& pattern : patterns)
                    {
                        if(entry_path_str.find(pattern) != std::string::npos)
                        {
                            logger::log(logger::level::INFO, logger::msgFormat("Skipping path due to ignore pattern: " + entry_path_str, __FUNCTION__, __FILE__, __LINE__));
                            return true;
                        }
                    }
                    return false;
                }(ignore_patterns);

                if(skip) continue;

                if(fs::is_directory(entry.path())) continue;
                
                if(!add(entry.path().string()))
                {
                    logger::log(logger::level::ERROR, logger::msgFormat("Failed to add file: " + entry.path().string(), __FUNCTION__, __FILE__, __LINE__));
                    return false;
                }

                paths.push_back(entry.path());
            }
        }

        for(const auto& path : paths)
        {
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

    /*
     * Name: sync
     * Description: Synchronizes staged file paths to the configured remote project
     * Parameters:
     *   project_name: Remote project directory name
     * Returns: True if rsync succeeds, false otherwise
     */
    bool sync(std::string project_name)
    {
        std::vector<Stage> stages;
        readStagingFile(stages);
        std::vector<fs::path> files_to_sync;

        for(const auto& stage  : stages)
        {
            fs::path file = bytesToPath(stage.path);
            logger::log(logger::level::INFO, logger::msgFormat("Syncing file: " + file.string(), __FUNCTION__, __FILE__, __LINE__));
            files_to_sync.push_back(file);
        }

        if(!rsync::rsync(project_name, files_to_sync))
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Failed to sync files to remote destination.", __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        logger::log(logger::level::INFO, "Successfully synced files to remote destination for project: " + project_name);
        return true;
    }
};