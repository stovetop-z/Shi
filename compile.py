import subprocess

# Define the command as a clean list of strings
compile_cmd = [
    "clang++", 
    "-std=c++23", 
    "-Wall", 
    "-Wextra", 
    "main.cc", 
    "-I/opt/homebrew/include",
    "-L/opt/homebrew/lib",
    "-I/opt/homebrew/opt/openssl@3/include", 
    "-L/opt/homebrew/opt/openssl@3/lib", 
    "-lcrypto", 
    "-lz", 
    "-lssh", 
    "-lboost_filesystem", 
    "-o", 
    "shi.out"
]

try:
    result = subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
    print("Compilation successful!")
except subprocess.CalledProcessError as e:
    print("Compilation failed!")
    print(e.stderr)
