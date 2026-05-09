#!/bin/bash

set -e

echo "=================================="
echo "        myshell test script        "
echo "=================================="

if [ ! -x ./myshell ]; then
    echo "Error: ./myshell not found."
    echo "Run: make first."
    exit 1
fi

rm -rf test_home
mkdir -p test_home
export HOME="$(pwd)/test_home"

rm -f test_output.txt append_test.txt commands_test.txt input_test.txt script_test

echo
echo "[1] Basic external commands"
echo "---------------------------"
echo "echo hello
pwd
date
" | ./myshell

echo
echo "[2] Command arguments"
echo "---------------------"
echo "echo one two three
ls -l
" | ./myshell

echo
echo "[3] cd command"
echo "--------------"
echo "pwd
cd /tmp
pwd
cd
pwd
" | ./myshell

echo
echo "[4] Output redirection: >"
echo "-------------------------"
echo "echo hello > test_output.txt
cat test_output.txt
" | ./myshell

echo "Expected file content: hello"
echo "Actual file content:"
cat test_output.txt

echo
echo "[5] Output redirection: >>"
echo "--------------------------"
echo "echo first > append_test.txt
echo second >> append_test.txt
cat append_test.txt
" | ./myshell

echo "Expected file content:"
echo "first"
echo "second"
echo "Actual file content:"
cat append_test.txt

echo
echo "[6] Background process and jobs"
echo "-------------------------------"
echo "sleep 3 &
jobs
" | ./myshell

echo
echo "[7] Finished background process should disappear"
echo "-----------------------------------------------"
echo "sleep 1 &
sleep 2
jobs
" | ./myshell

echo
echo "[8] Command history"
echo "-------------------"
echo "echo history_test_1
echo history_test_2
echo history_test_3
" | ./myshell

echo "History file:"
cat "$HOME/.myshell_history"

echo
echo "[9] History limit: last 20 commands"
echo "-----------------------------------"
for i in $(seq 1 25); do
    echo "echo history_number_$i"
done | ./myshell > /dev/null

echo "Number of lines in history file:"
wc -l "$HOME/.myshell_history"

echo
echo "History file should contain commands history_number_6 to history_number_25:"
cat "$HOME/.myshell_history"

echo
echo "[10] Script file passed as argument"
echo "-----------------------------------"
cat > commands_test.txt <<EOF
echo script_argument_started
pwd
echo script_argument_finished
EOF

./myshell commands_test.txt

echo
echo "[11] Script with shebang"
echo "------------------------"
SCRIPT_PATH="$(pwd)/myshell"

cat > script_test <<EOF
#!$SCRIPT_PATH
echo shebang_script_started
pwd
echo shebang_script_finished
EOF

chmod +x script_test
./script_test

echo
echo "[12] Input from file using <"
echo "----------------------------"
cat > input_test.txt <<EOF
echo input_file_started
pwd
echo input_file_finished
EOF

./myshell < input_test.txt

echo
echo "[13] Syslog option -l"
echo "---------------------"
echo "echo syslog_short_option
" | ./myshell -l

echo
echo "[14] Syslog option --syslog"
echo "---------------------------"
echo "echo syslog_long_option
" | ./myshell --syslog

echo
echo "[15] Error handling: unknown command"
echo "------------------------------------"
echo "this_command_does_not_exist
" | ./myshell || true

echo
echo "[16] Error handling: invalid cd"
echo "-------------------------------"
echo "cd directory_that_does_not_exist
" | ./myshell || true

echo
echo "[17] Error handling: missing file after >"
echo "-----------------------------------------"
echo "echo test >
" | ./myshell || true

echo
echo "[18] EOF handling"
echo "-----------------"
printf "echo before_eof\n" | ./myshell

echo
echo "=================================="
echo "Tests finished."
echo "Please review the output above."
echo "=================================="

rm -f test_output.txt append_test.txt commands_test.txt input_test.txt script_test
