compile = "clang++ -std=c++23 -Wall -Wextra main.cc \
  -I/opt/homebrew/opt/openssl@3/include \
  -L/opt/homebrew/opt/openssl@3/lib \
  -lcrypto -lz -lssh -lboost_system -o shi"

import os

os.subprocess(compile)