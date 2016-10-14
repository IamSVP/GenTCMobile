#ifndef __MPTC_DECODER_H__
#define __MPTC_DECODER_H__


#include "arithmetic_codec.h"

#include <fstream>
#include <tuple>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstring>
#include <atomic>
#include <typeinfo>
#include <cassert>
#include <android/log.h>

#define LOG_TAG "GenTCJNI"
#ifdef  DEBUG
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

#else
#define ALOGE(...) //
#define ALOGV(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif

union PhysicalDXTBlock {
  struct {
    uint16_t ep1;
    uint16_t ep2;
    uint32_t interp;
  };
  uint64_t dxt_block;
};

struct LogicalDXTBlock {
  uint8_t ep1[4];
  uint8_t ep2[4];
  uint8_t palette[4][4];
  uint8_t indices[16];
  LogicalDXTBlock &operator=(const LogicalDXTBlock &other) {
    memcpy(this, &other, sizeof(*this));
    return *this;
  }

  bool operator==(const LogicalDXTBlock &other) const {
    return memcmp(this, &other, sizeof(*this)) == 0;
  }

};


// structure to remember previous decode info
// Should be passed to the decode function to get the next frame
typedef struct _DecodeInfo {

  uint32_t frame_height, frame_width, total_frame_count;
  uint8_t unique_interval, search_area;
  uint8_t curr_idx; // current frame number within the unique interval, this is less than < unique_interval

  bool is_start, is_unique;

  std::vector<uint8_t> comp_palette, uncomp_palette;
  std::vector<uint8_t> comp_motion_indices, motion_indices;
  std::vector<uint8_t> comp_ep_Y, comp_ep_C;
  std::vector<uint8_t> comp_ep1_Y, comp_ep1_C, comp_ep2_Y, comp_ep2_C;
  std::vector<uint8_t > wav_ep1_Y, wav_ep1_C, wav_ep2_Y, wav_ep2_C;
  std::vector<int8_t>  ep1_Y, ep1_Co, ep1_Cg, ep2_Y, ep2_Co, ep2_Cg;
  uint32_t num_blocks, unique_idx_offset;
  uint32_t max_unique_count; // The maximum size of the uncompressed dictionary as uint8_t entries
  uint32_t max_compressed_palette, max_compressed_motion_indices;  // a variable to be stored so we can avoid reallocing dictionary
  uint64_t curr_frame;

  uint32_t max_compressed_ep_C, max_compressed_ep_Y;
  bool is_multi_thread;

                                     // every time a new unique dictionary has to be read
    _DecodeInfo(){

     };
    ~_DecodeInfo() {


    }
} MPTCDecodeInfo;


typedef struct _BufferStruct {

  MPTCDecodeInfo *ptr_decode_info;
  PhysicalDXTBlock *curr_dxt, *prev_dxt;
  PhysicalDXTBlock **buffered_dxts;
  uint8_t buffer_sz;
  uint8_t curr_dxt_idx; // points to the current dxt to be returned
  uint8_t curr_decode_idx; // points to the decode pointer to be filled in
  uint8_t prev_decode_idx;
  _BufferStruct() {
      ptr_decode_info = NULL;
      buffered_dxts = NULL;
  }
  ~_BufferStruct() {
      if(ptr_decode_info) delete ptr_decode_info;
      for(uint8_t i = 0; i < buffer_sz; i++)
          delete buffered_dxts[i];
  }
} BufferStruct;


int GetFrame(std::ifstream &in_stream, PhysicalDXTBlock *prev_dxt, PhysicalDXTBlock *curr_dxt,
              MPTCDecodeInfo *decode_info);

void GetFrameMultiThread(std::ifstream &in_stream, PhysicalDXTBlock *prev_dxt, PhysicalDXTBlock *curr_dxt,
                         MPTCDecodeInfo *decode_info);

int InitBufferedDecode(uint8_t buffer_sz, BufferStruct* &ptr_buffer_struct, std::ifstream &in_stream, uint32_t num_blocks);

int GetBufferedFrame(BufferStruct *ptr_buffer_struct, PhysicalDXTBlock * &curr_dxt, std::ifstream  &in_stream);

#endif 
