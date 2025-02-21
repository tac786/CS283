#!/usr/bin/env bats

# File: student_tests.sh

# ------------------------------------------------------------------------------
# basic command execution tests
# ------------------------------------------------------------------------------

@test "Basic: ls command" {
  run ./dsh <<EOF
ls
exit
EOF
  [ "$status" -eq 0 ]  # expect successful execution
}

@test "Basic: pwd command" {
  run ./dsh <<EOF
pwd
exit
EOF
  [[ "$output" =~ ^/.*$ ]]  # check if output starts with '/'
  [ "$status" -eq 0 ]
}

# ------------------------------------------------------------------------------
# built-in command tests
# ------------------------------------------------------------------------------

@test "Built-in: cd changes directory" {
  run ./dsh <<EOF
cd /tmp
pwd
exit
EOF
  [[ "$output" == *"/tmp"* ]]  # check if 'pwd' reflects '/tmp'
  [ "$status" -eq 0 ]
}

@test "Built-in: cd with no arguments" {
  run ./dsh <<EOF
cd
pwd
exit
EOF
  [[ "$output" =~ ^/.*$ ]]  # should remain in some valid directory
  [ "$status" -eq 0 ]
}

@test "Built-in: dragon prints ASCII art" {
  run ./dsh <<EOF
dragon
exit
EOF
  [[ "$output" == *"@"* ]]  # quick check for part of ASCII dragon
  [ "$status" -eq 0 ]
}

# ------------------------------------------------------------------------------
# quoting and spacing
# ------------------------------------------------------------------------------

@test "Quoting: quoted arguments preserve spaces" {
  run ./dsh <<EOF
echo "hello     world"
exit
EOF
  [[ "$output" == *"hello     world"* ]]
  [ "$status" -eq 0 ]
}

@test "Spacing: multiple spaces outside quotes" {
  run ./dsh <<EOF
echo    hello      world
exit
EOF
  [[ "$output" == *"hello world"* ]]
  [ "$status" -eq 0 ]
}

# ------------------------------------------------------------------------------
# pipeline tests
# ------------------------------------------------------------------------------

@test "Pipeline: ls | grep dsh" {
  run ./dsh <<EOF
ls | grep dsh
exit
EOF
  # expect to see some files like dshlib.c, dsh_cli.c, or similar
  [[ "$output" == *"dsh"* ]]
  [ "$status" -eq 0 ]
}

# ------------------------------------------------------------------------------
# redirection tests
# ------------------------------------------------------------------------------

@test "Redirection: output redirection with >" {
  run ./dsh <<EOF
echo "Hello World" > test_out.txt
cat test_out.txt
exit
EOF
  [[ "$output" == *"Hello World"* ]]
  [ "$status" -eq 0 ]
}

@test "Redirection: append with >>" {
  run ./dsh <<EOF
echo "Line 1" > test_out.txt
echo "Line 2" >> test_out.txt
cat test_out.txt
exit
EOF
  [[ "$output" == *"Line 1"* ]]
  [[ "$output" == *"Line 2"* ]]
  [ "$status" -eq 0 ]
}

@test "Redirection: input redirection with <" {
  # create a file in the test, feed it to cat
  echo "CS283 is fun" > in.txt
  run ./dsh <<EOF
cat < in.txt
exit
EOF
  [[ "$output" == *"CS283 is fun"* ]]
  [ "$status" -eq 0 ]
  rm in.txt
}

# ------------------------------------------------------------------------------
# exit command test
# ------------------------------------------------------------------------------

@test "Built-in: exit prints 'exiting...' and ends with 'cmd loop returned 0'" {
  run ./dsh <<EOF
exit
EOF
  [[ "$output" == *"exiting..."* ]]
  [[ "$output" == *"cmd loop returned 0"* ]]
  [ "$status" -eq 0 ]
}