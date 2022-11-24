#!/bin/bash

TEST_BIN_PATH="../target/bin/test/unit"
TEST_SRC_PATH="test/unit"
INCLUDE_PATH="include"

CC_FLAGS="-m32 -g1 -O2 -Wall -Wno-address-of-packed-member -D_GNU_SOURCE"
CPP_FLAGS="-I $INCLUDE_PATH"

declare -A DEPENDENCY_MODULES_BY_TEST
DEPENDENCY_MODULES_BY_TEST["test_wildcard_pattern_matcher"]="user/util/wildcard_pattern_matcher.c"
DEPENDENCY_MODULES_BY_TEST["test_unrolled_linked_list"]="common/util/unrolled_linked_list.c"
DEPENDENCY_MODULES_BY_TEST["test_ring_buffer"]="common/util/ring_buffer.c"
DEPENDENCY_MODULES_BY_TEST["test_priority_queue"]="common/util/priority_queue.c"
DEPENDENCY_MODULES_BY_TEST["test_double_linked_list"]="common/util/double_linked_list.c"
DEPENDENCY_MODULES_BY_TEST["test_checksum"]="user/util/checksum.c"
DEPENDENCY_MODULES_BY_TEST["test_simple_memory_allocator"]="common/util/double_linked_list.c user/util/checksum.c user/util/simple_memory_allocator.c"
DEPENDENCY_MODULES_BY_TEST["test_date_time_utils"]="common/util/date_time_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_formatter"]="common/util/formatter.c common/util/scanner.c user/util/scanner.c common/util/stream_writer.c common/util/string_stream_writer.c common/util/stream_reader.c common/util/string_stream_reader.c user/util/dynamic_array.c common/util/math_utils.c common/util/date_time_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_string_utils"]="common/util/string_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_scanner"]="common/util/scanner.c user/util/scanner.c common/util/stream_reader.c common/util/string_stream_reader.c user/util/dynamic_array.c common/util/math_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_b_tree"]="common/util/b_tree.c common/util/search_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_string_stream_reader"]="common/util/stream_reader.c common/util/string_stream_reader.c"
DEPENDENCY_MODULES_BY_TEST["test_file_descriptor_reader"]="common/util/stream_reader.c user/util/file_descriptor_stream_reader.c"
DEPENDENCY_MODULES_BY_TEST["test_buffered_stream_reader"]="common/util/stream_reader.c common/util/string_stream_reader.c common/util/string_utils.c user/util/file_descriptor_stream_reader.c user/util/buffered_stream_reader.c"
DEPENDENCY_MODULES_BY_TEST["test_path_utils"]="common/util/path_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_dynamic_array"]="user/util/dynamic_array.c"
DEPENDENCY_MODULES_BY_TEST["test_overflow_proof_integer"]="common/util/overflow_proof_integer.c"
DEPENDENCY_MODULES_BY_TEST["test_search_utils"]="common/util/search_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_fixed_capacity_sorted_array"]="common/util/fixed_capacity_sorted_array.c common/util/search_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_command_line_utils"]="common/util/command_line_utils.c common/util/string_stream_writer.c common/util/stream_writer.c common/util/formatter.c common/util/scanner.c common/util/stream_reader.c common/util/string_stream_reader.c user/util/dynamic_array.c common/util/math_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_string_stream_writer"]="common/util/stream_writer.c common/util/string_stream_writer.c common/util/formatter.c common/util/scanner.c common/util/stream_reader.c common/util/string_stream_reader.c user/util/dynamic_array.c common/util/math_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_file_descriptor_writer"]="common/util/stream_writer.c common/util/formatter.c common/util/scanner.c common/util/stream_reader.c common/util/string_stream_reader.c user/util/dynamic_array.c user/util/file_descriptor_stream_writer.c common/util/math_utils.c"
DEPENDENCY_MODULES_BY_TEST["test_buffered_stream_writer"]="common/util/stream_writer.c common/util/formatter.c common/util/scanner.c common/util/stream_reader.c common/util/string_stream_reader.c user/util/dynamic_array.c user/util/file_descriptor_stream_writer.c user/util/buffered_stream_writer.c common/util/math_utils.c"

rm -rf $TEST_BIN_PATH
mkdir -p $TEST_BIN_PATH

set -e

# Find all tests and related files.
for test_file_path in `find $TEST_SRC_PATH -iname 'test_*' | sort`; do
   test_file_name=$(basename -- "$test_file_path")
   test_file_name_without_extension="${test_file_name%.*}"

   echo "$test_file_name_without_extension"
	
   gcc $CC_FLAGS $CPP_FLAGS $test_file_path  \
      ${DEPENDENCY_MODULES_BY_TEST[$test_file_name_without_extension]} \
      -o "$TEST_BIN_PATH/$test_file_name_without_extension"
	"$TEST_BIN_PATH/$test_file_name_without_extension"
done
