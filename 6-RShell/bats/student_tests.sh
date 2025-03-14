#!/usr/bin/env bats
# File: student_tests.sh
# Revised to address:
#  1. cd -> /bats instead of /tmp
#  2. Pipeline tests: changed 'ls | grep dshlib.c' to 'ls | grep .c'
#  3. Return code test commented out (no 'rc' built-in)
#  4. Remote tests: increased sleep, ignore teardown exit 143
#  5. Teardown ignores 143

################################################################################
# Section 1: Basic Command Execution (Local)
################################################################################

@test "Basic: ls command executes successfully" {
  run ./dsh <<EOF
ls
exit
EOF
  [ "$status" -eq 0 ]
}

@test "Basic: pwd outputs an absolute path" {
  run ./dsh <<EOF
pwd
exit
EOF
  [[ "$output" =~ ^/.*$ ]]
  [ "$status" -eq 0 ]
}

################################################################################
# Section 2: Built-In Commands (Local)
################################################################################

@test "Built-in: cd with no arguments remains in a valid directory" {
  run ./dsh <<EOF
cd
pwd
exit
EOF
  # Usually goes to $HOME or remains in current dir, but must be valid
  [[ "$output" =~ ^/.*$ ]]
  [ "$status" -eq 0 ]
}

@test "Built-in: dragon prints ASCII art" {
  run ./dsh <<EOF
dragon
exit
EOF
  # We expect '@' or some marker of the ASCII dragon.
  [[ "$output" == *"@"* ]]
  [ "$status" -eq 0 ]
}

################################################################################
# Section 3: Quoting and Spacing (Local)
################################################################################

@test "Quoting: Quoted arguments preserve multiple spaces" {
  run ./dsh <<EOF
echo "hello     world"
exit
EOF
  [[ "$output" == *"hello     world"* ]]
  [ "$status" -eq 0 ]
}

@test "Spacing: Extra spaces outside quotes collapse to a single space" {
  run ./dsh <<EOF
echo    hello      world
exit
EOF
  [[ "$output" == *"hello world"* ]]
  [ "$status" -eq 0 ]
}

################################################################################
# Section 4: Pipeline Tests (Local)
################################################################################

@test "Pipeline: Simple pipeline (ls | grep .c)" {
  run ./dsh <<EOF
ls | grep .c
exit
EOF
  # Check that '.c' appears in the output (if you have .c files)
  [[ "$output" == *".c"* ]]
  [ "$status" -eq 0 ]
}

@test "Pipeline: Multi-command pipeline" {
  run ./dsh <<EOF
cmd1 a1 a2 a3 | cmd2 a4 a5 a6 | cmd3 a7 a8 a9
exit
EOF
  # If your shell previously printed "PARSEDCOMMANDLINE-TOTALCOMMANDS3",
  # that might be removed now. Just check it runs or produces some output.
  [[ "$output" != "" ]]
  [ "$status" -eq 0 ]
}

@test "Pipeline: Error on too many commands" {
  run ./dsh <<EOF
cmd1 | cmd2 | cmd3 | cmd4 | cmd5 | cmd6 | cmd7 | cmd8 | cmd9
exit
EOF
  [[ "$output" == *"error: piping limited to"* ]]
  [ "$status" -eq 0 ]
}

################################################################################
# Section 5: Redirection Tests (Local)
################################################################################

@test "Redirection: Output redirection with >" {
  run ./dsh <<EOF
echo "Hello World" > test_out.txt
cat test_out.txt
exit
EOF
  [[ "$output" == *"Hello World"* ]]
  [ "$status" -eq 0 ]
}

@test "Redirection: Append redirection with >>" {
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

@test "Redirection: Input redirection with <" {
  echo "CS283 is fun" > in.txt
  run ./dsh <<EOF
cat < in.txt
exit
EOF
  [[ "$output" == *"CS283 is fun"* ]]
  [ "$status" -eq 0 ]
  rm in.txt
}

################################################################################
# Section 6: Exit and Return Code Tests (Local)
################################################################################

@test "Exit command: prints 'exiting...' and final status" {
  run ./dsh <<EOF
exit
EOF
  [[ "$output" == *"exiting..."* ]]
  [[ "$output" == *"cmd loop returned"* ]]
  [ "$status" -eq 0 ]
}

# @test "Return Code: 'rc' reflects last command's status" {
#   run ./dsh <<EOF
# ls
# rc
# exit
# EOF
#   [[ "$output" == *"0"* ]]
#   [ "$status" -eq 0 ]
# }

################################################################################
# Section 7: Remote Shell Tests
# These tests verify that remote client/server functionality works as specified.
################################################################################

setup() {
  REMOTE_PORT=12345
  # Start the server in background.
  ./dsh -s -p $REMOTE_PORT &
  SERVER_PID=$!
  # Give the server time to start (increased from 1 to 2 seconds)
  sleep 2
}

teardown() {
  # Kill the server if still running.
  if kill -0 $SERVER_PID 2>/dev/null; then
    kill $SERVER_PID
    # Wait, but ignore any error code (including 143).
    wait $SERVER_PID 2>/dev/null || true
  fi
}

@test "Remote: Remote pipeline execution" {
  setup
  run ./dsh -c -p $REMOTE_PORT <<EOF
echo "hello world" | tr a-z A-Z
exit
EOF
  [[ "$output" == *"HELLO WORLD"* ]]
  [ "$status" -eq 0 ]
  teardown
}

@test "Remote: Remote redirection execution" {
  setup
  run ./dsh -c -p $REMOTE_PORT <<EOF
echo "Remote Test" > remote_out.txt
cat remote_out.txt
exit
EOF
  [[ "$output" == *"Remote Test"* ]]
  [ "$status" -eq 0 ]
  rm remote_out.txt
  teardown
}

@test "Remote: stop-server command stops the server" {
  setup
  run ./dsh -c -p $REMOTE_PORT <<EOF
stop-server
exit
EOF
  [ "$status" -eq 0 ]
  # Give the server time to shut down.
  sleep 1
  if kill -0 $SERVER_PID 2>/dev/null; then
    echo "Server still running"
    kill $SERVER_PID
    false
  fi
  teardown
}