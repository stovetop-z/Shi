#pragma once
#include <sstream>
#include <cstddef>
#include <fstream>
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

    using namespace args; 

    struct File
    {
        fs::path file_path;
        int size;
        std::vector<Byte> raw_content;
    };

    struct Shi
    {
        fs::path shi_path;
        File src_file;
        std::string blob_raw;
        std::string blob_hash;
        std::string type;
    };

    struct TreeShi
    {
        std::vector<Shi*> shis;
        std::string branch;
    };

    struct HashRes
    {
        std::string dir;
        std::string file_hash;
        std::string full_hash;
    };

    std::string inline hashToString(const Byte* hash, size_t length)
    {
        std::ostringstream ss;
        for(size_t i = 0; i < length; i++)
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        return ss.str();
    }

    inline HashRes sha256(const std::string& to_hash) 
    {
        unsigned char shaed[SHA256_DIGEST_LENGTH];
        SHA256((const unsigned char*)to_hash.c_str(), to_hash.length(), shaed);
        
        std::string full_hash = hashToString(shaed, SHA256_DIGEST_LENGTH);

        return HashRes{
            .dir = full_hash.substr(0, 2),
            .file_hash = full_hash.substr(2, SHA256_DIGEST_LENGTH - 2),
            .full_hash = full_hash
        };
    }

    inline File getFile(const fs::path& f)
    {
        if(!fs::is_regular_file(f)) return File();

        File new_file;
        new_file.file_path = f;
        new_file.size = fs::file_size(f);
        
        std::ifstream open_file(f);
        std::stringstream buff;
        buff << open_file.rdbuf();
        open_file.close();

        std::string content = buff.str();
        new_file.raw_content = std::vector<Byte>(content.begin(), content.end());
        return new_file;
    }

    inline bool isShiDir(const fs::path& p)
    {
        return fs::exists(p / SHI_DIR) && fs::is_directory(p / SHI_DIR);
    }

    inline std::vector<Byte> compressShi(const std::vector<Byte>& data)
    {
        if(data.empty()) return {};

        uLong src_len = static_cast<uLong>(data.size());
        uLongf dest_len = compressBound(src_len);
        std::vector<Byte> dest(dest_len);

        int res = compress(dest.data(), &dest_len, data.data(), src_len);
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
        
        if(!raw_content.empty() && compressed_data.empty())
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

        if(!compressed_data.empty())
        {
            blob_file.write(reinterpret_cast<const char*>(compressed_data.data()), static_cast<std::streamsize>(compressed_data.size()));
        }
        
        blob_file.close();

        logger::log(logger::level::INFO, "Successfully created blob file at: " + shi_path.string());
        return true;
    }

    inline bool addToTree(Shi* shi)
    {
        
    }
 
    Shi blobbify(const fs::path& p)
    {
        File f = getFile(p);
        Shi blob_obj;
        blob_obj.src_file = f;

        // Construct object header: "blob <size>\0<content>"
        std::string header = "blob " + std::to_string(f.size) + '\0';

        blob_obj.blob_raw.reserve(header.size() + f.raw_content.size());
        blob_obj.blob_raw.insert(blob_obj.blob_raw.end(), header.begin(), header.end());
        blob_obj.blob_raw.insert(blob_obj.blob_raw.end(), f.raw_content.begin(), f.raw_content.end());

        HashRes hash_res = sha256(blob_obj.blob_raw);
        blob_obj.blob_hash = hash_res.full_hash;
        blob_obj.shi_path = fs::path(SHI_OBJECTS_DIR) / hash_res.dir / hash_res.file_hash;

        // We need to add to the tree file
        addToTree(&blob_obj);
        
        return blob_obj;
    }

    inline bool createTree(const std::string&& branch)
    {

    }

    inline bool createMetadata(fs::path* p)
    {
        fs::path paths = *p;


        std::ifstream file(".mtd");
    }

    bool init(const std::string& arg = "")
    {
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
        
        logger::log(logger::level::INFO, "Successfully initialized .shi directory at: " + init_path.string());

        createMetadata();

        createTree("master");

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

        Shi shi = blobbify(arg);
        return createBlobFile(shi);
    }

    bool sync(std::string project_name, const std::vector<std::string>& args)
    {
        std::vector<fs::path> files_to_sync(args.begin(), args.end());
        if(sync::rsync(project_name, files_to_sync))
        {
            logger::log(logger::level::ERROR, logger::msgFormat("Failed to sync files to remote destination.", __FUNCTION__, __FILE__, __LINE__));
            return false;
        }

        logger::log(logger::level::INFO, "Successfully synced files to remote destination for project: " + project_name);
        return true;
    }
};
