#!/usr/bin/env bats

# File: student_tests.sh
# 
# Custom unit tests for dsh

# Basic command execution tests
# This test ensures that the 'ls' command runs successfully within dsh
@test "Basic: ls command" {
    run ./dsh <<EOF
ls
EOF
    [ "$status" -eq 0 ]  # Expect successful execution
}

# Ensure `pwd` outputs the correct working directory
# The output should be an absolute path starting with '/' ensuring correct behavior
@test "Basic: pwd command" {
    run ./dsh <<EOF
pwd
EOF
    [[ "$output" =~ ^/.*$ ]]  # Check if output starts with '/'
    [ "$status" -eq 0 ]
}

# 'rc' should return '0' after a successful 'ls' execution.
@test "Return Code: rc after successful command" {
    run ./dsh <<EOF
ls
rc
EOF
    [[ "$output" == *"0"* ]]  # Expect last return code to be 0
}

# This test verifies that 'cd /tmp' correctly updates the working directory
@test "Built-in: cd command changes directory" {
    run ./dsh <<EOF
cd /tmp
pwd
EOF
    [[ "$output" == *"/tmp"* ]]  # Check if 'pwd' reflects '/tmp'
}

# 'cd' with no arguments should stay in the same directory
@test "Built-in: cd with no arguments" {
    run ./dsh <<EOF
cd
pwd
EOF
    [[ "$output" =~ ^/.*$ ]]
}

# Running 'dragon' should output ASCII art
@test "Built-in: dragon command prints ASCII" {
    run ./dsh <<EOF
dragon
EOF
    [[ "$output" == *"@"* ]]  # Check if ASCII dragon is printed
}

# This test verifies that multiple spaces inside quotes are preserved.
@test "Handling: Quoted arguments preserve spaces" {
    run ./dsh <<EOF
echo "hello     world"
EOF
    [[ "$output" == *"hello     world"* ]]  # Spaces inside quotes should remain unchanged
}

# Extra spaces between words should be reduced to a single space
@test "Handling: Preserve multiple spaces outside quotes" {
    run ./dsh <<EOF
echo    hello      world
EOF
    [[ "$output" == *"hello world"* ]]  # Verify that spaces are collapsed
}

# 'rc' should reflect the return code of the last executed command
@test "Return Code: rc updates after success" {
    run ./dsh <<EOF
ls
rc
EOF
    [[ "$output" == *"0"* ]]  # Expect 0 return code after 'ls'
}