#!/bin/bash

echo "Compiling project..."
make

echo
echo "Test 1: basic commands"
echo "pwd
echo hello
" | ./myshell

echo
echo "Test 2: output redirection >"
echo "echo hello > test_output.txt
cat test_output.txt
" | ./myshell

echo
echo "Test 3: output redirection >>"
echo "echo first > append_test.txt
echo second >> append_test.txt
cat append_test.txt
" | ./myshell

echo
echo "Test 4: cd command"
echo "pwd
cd /tmp
pwd
" | ./myshell

echo
echo "Test 5: background process and jobs"
echo "sleep 2 &
jobs
" | ./myshell

echo
echo "Test 6: history file"
echo "echo history_test_1
echo history_test_2
" | ./myshell

echo
echo "Checking history file:"
tail -n 5 ~/.myshell_history

echo
echo "All basic tests finished."
