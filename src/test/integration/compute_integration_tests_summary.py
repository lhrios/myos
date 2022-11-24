#!/usr/bin/python3

import os
import sys
from os import listdir
from pathlib import Path

def _print_section_break(content):
	(columns, rows) = os.get_terminal_size(0)

	print()
	if content:
		before = (columns - len(content) - 2) // 2
	else:
		before = (columns) // 2
	if before > 0:
		print("*" * before, end="")

	if content:
		print(" " + content + " ", end="")

	if content:	
		after = (columns - before - len(content) - 2)
	else:
		after = (columns - before)
	if after > 0:
		print("*" * after, end="")

def _main(argv):
	if not argv or len(argv) != 3:
		print("Wrong number of arguments\n")
	else:
		test_case_executables_path = argv[1]
		test_case_output_path = argv[2]
	
		succeeded_by_test_case_name = dict()
		for executable_name in listdir(test_case_executables_path):
			if executable_name.startswith("test_"):
				succeeded_by_test_case_name[executable_name] = False

		SUCCEEDED_SUFFIX = ".succeeded"

		succeeded_count = 0
		for output_name in listdir(test_case_output_path):
			if output_name.endswith(SUCCEEDED_SUFFIX):
				succeeded_by_test_case_name[output_name[: -len(SUCCEEDED_SUFFIX)]] = True
				succeeded_count += 1
				
		_print_section_break("Integration test execution summary");

		for (executable_name, succeeded) in sorted(succeeded_by_test_case_name.items(), key=lambda i: i[0]):
			if not succeeded:
				print("\x1B[3m" + executable_name + "\x1B[m: ")
				path = Path(test_case_output_path + "/" + executable_name + ".txt")
				if path.is_file():
					print(path)
					with open(str(path), 'r') as output_file:
						print(output_file.read())
				else:
					print("\tNo ouput file\n")

		for (executable_name, succeeded) in sorted(succeeded_by_test_case_name.items(), key=lambda i: i[0]):
			print("\x1B[3m" + executable_name + "\x1B[m: ", end="")
			if not succeeded:
				print("\x1B[91mfailed\x1B[m")
			else:
				print("\x1B[32msucceeded\x1B[m")
		
		print()
		if len(succeeded_by_test_case_name) != succeeded_count:
			print("\x1B[91mSome test cases have failed!\x1B[m")
			_print_section_break("")
			sys.exit(1)
		else:
			print("\x1B[32mAll test cases have succeeded!\x1B[m")
			_print_section_break("")
			sys.exit(0)


if __name__ == '__main__':
	_main(sys.argv)
