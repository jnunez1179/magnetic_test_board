cmake_minimum_required(VERSION 3.28.3)
find_package(Zephyr REQUIRED HINTS $ENV{ZERPHYR_BASE})
project(mtb)

target_sources(app PRIVATE
  src/iso_tp.c
  proto/mtb.pb.c
)

target_include_directories(app PRIVATE
  inc
  proto
)
