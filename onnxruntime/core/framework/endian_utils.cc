// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/framework/endian_utils.h"

#include <cassert>
#include <cstring>
#include <altivec.h>
#include "core/common/endian.h"
#include <vector>
namespace onnxruntime {
namespace utils {

namespace {

// analogous to std::reverse_copy
template <typename BidirIt, typename OutputIt>
OutputIt ReverseCopy(BidirIt first, BidirIt last, OutputIt d_first) {
  while (last != first) {
    --last;
    *d_first = *last;
    ++d_first;
  }
  return d_first;
}

}  // namespace


vector unsigned char CreateMask(size_t elem_size)
{
  // storing the elements into a temporary array,
  alignas(16) unsigned char mask[16];
  size_t org_elem_size=elem_size;
  int cntr=0;
  size_t times = 16/elem_size;
  while(times>0)
  {
    for(size_t i=0;i<4;i++)
    {
      mask[cntr++]=(unsigned char)(org_elem_size -1 - i);
    }
    times--;
    org_elem_size+=elem_size;
  }
  return vec_vsx_ld(0, mask);
}



void SwapByteOrderCopy(size_t element_size_in_bytes,
                       gsl::span<const unsigned char> source_bytes,
                       gsl::span<unsigned char> destination_bytes) {
  assert(element_size_in_bytes > 0);
  assert(source_bytes.size_bytes() % element_size_in_bytes == 0);
  assert(source_bytes.size_bytes() == destination_bytes.size_bytes());
  // check non-overlapping
  // given begin <= end, end0 <= begin1 || end1 <= begin0
  assert(source_bytes.data() + source_bytes.size() <= destination_bytes.data() ||
         destination_bytes.data() + destination_bytes.size() <= source_bytes.data());
if(element_size_in_bytes>1)
         {vector unsigned char mask =  CreateMask(element_size_in_bytes);
         size_t element_offset = 0;
  for (size_t element_offset_end = source_bytes.size_bytes();
       element_offset + 16 < element_offset_end;
       element_offset += 16) {
    const auto source_element_bytes = source_bytes.subspan(element_offset, 16);
    const auto dest_element_bytes = destination_bytes.subspan(element_offset, 16);
    vector unsigned char v_data = vec_vsx_ld(0, (unsigned char*)source_element_bytes.data());
    vector unsigned char v_swapped = vec_perm(v_data, v_data,mask);
    vec_vsx_st(v_swapped, 0, (unsigned char*)dest_element_bytes.data());
  
    // ReverseCopy(source_element_bytes.data(),
    //             source_element_bytes.data() + source_element_bytes.size_bytes(),
    //             dest_element_bytes.data());
  }

  // for the remaining bytes which are not multiple of 16
  for (; element_offset < source_bytes.size_bytes(); element_offset += element_size_in_bytes)
  {
    const auto source_element_bytes = source_bytes.subspan(element_offset, element_size_in_bytes);
    const auto dest_element_bytes = destination_bytes.subspan(element_offset, element_size_in_bytes);
    ReverseCopy(source_element_bytes.data(),
                source_element_bytes.data() + source_element_bytes.size_bytes(),
                dest_element_bytes.data());
  }
         }
         else{
             for (size_t element_offset=0; element_offset < source_bytes.size_bytes(); element_offset += element_size_in_bytes)
  {
    const auto source_element_bytes = source_bytes.subspan(element_offset, element_size_in_bytes);
    const auto dest_element_bytes = destination_bytes.subspan(element_offset, element_size_in_bytes);
    ReverseCopy(source_element_bytes.data(),
                source_element_bytes.data() + source_element_bytes.size_bytes(),
                dest_element_bytes.data());
  }
         }
}




void SwapByteOrderInplace(size_t element_size_in_bytes, gsl::span<std::byte> bytes) {
  ORT_ENFORCE(element_size_in_bytes > 0, "Expecting a positive element size");
  ORT_ENFORCE(bytes.size_bytes() % element_size_in_bytes == 0, "Expecting a match");
  if (element_size_in_bytes > 1) {
    vector unsigned char mask = CreateMask(element_size_in_bytes);

    size_t offset=0;
    for ( size_t lim = bytes.size_bytes(); offset +16 < lim; offset += 16) {
        vector unsigned char v_data = vec_vsx_ld(0, (unsigned char*)&bytes[offset]);
        vector unsigned char v_swapped = vec_perm(v_data, v_data,mask);
        vec_vsx_st(v_swapped, 0, (unsigned char*)&bytes[offset]);
      // std::reverse(bytes.begin() + offset, bytes.begin() + offset + element_size_in_bytes);
    }

    // for the remaining bytes which are not multiple of 16
    for(;offset<bytes.size_bytes();offset+=element_size_in_bytes)
    {
      std::reverse(bytes.begin() + offset, bytes.begin() + offset + element_size_in_bytes);
    }
  }
}

namespace detail {

Status CopyLittleEndian(size_t element_size_in_bytes,
                        gsl::span<const unsigned char> source_bytes,
                        gsl::span<unsigned char> destination_bytes) {
  ORT_RETURN_IF(source_bytes.size_bytes() != destination_bytes.size_bytes(),
                "source and destination buffer size mismatch");

  if constexpr (endian::native == endian::little) {
    std::memcpy(destination_bytes.data(), source_bytes.data(), source_bytes.size_bytes());
  } else {
    SwapByteOrderCopy(element_size_in_bytes, source_bytes, destination_bytes);
  }

  return Status::OK();
}

}  // namespace detail

common::Status ReadLittleEndian(size_t element_size,
                                gsl::span<const unsigned char> source_bytes,
                                gsl::span<unsigned char> destination_bytes) {
  return detail::CopyLittleEndian(element_size, source_bytes, destination_bytes);
}

}  // namespace utils
}  // namespace onnxruntime
