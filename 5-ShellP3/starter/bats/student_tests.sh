#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Multiple Pipelines" {
    run "./dsh" <<EOF
ls | cat dshlib.h | wc -l
EOF

    # The expected output should be the count of `.c` files in the directory
    # (Change `expected_output` based on the actual number of `.c` files in the directory)
    expected_output="92"

    # Assertions
    [ "$status" -eq 0 ]
    [ "$output" = "$expected_output" ]
}
@test "Basic Command - echo" {
    run "./dsh" <<EOF
echo Hello, World!
EOF

    # Expected output
    expected_output="Hello, World!"

    # Assertions
    [ "$status" -eq 0 ]
    [ "$output" = "$expected_output" ]
}
@test "Invalid Command" {
    run "./dsh" <<EOF
nonexistentcommand
EOF

    # Expected output should indicate the command was not found
    expected_output="Error executing command: nonexistentcommand"

    # Assertions
    [ "$status" -ne 0 ]
    echo "$output" | grep -q "$expected_output"  # Check if error message is in output
}