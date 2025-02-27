#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Multiple Pipelines" {
    run "./dsh" <<EOF
ls | cat dshlib.h | wc -l
EOF

 stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="92dsh3>dsh3>cmdloopreturned0"

    # Assertions
    [ "$status" -eq 0 ]
    [ "$output" = "$expected_output" ]
}
@test "Basic Command - echo" {
    run "./dsh" <<EOF
echo Hello, World!
EOF
stripped_output=$(echo "$output" | tr -d '[:space:]')
    # Expected output
    expected_output="Hello, World!dsh3>dsh3>cmdloopreturned0"

    # Assertions
    [ "$status" -eq 0 ]
    [ "$output" = "$expected_output" ]
}
@test "Invalid Command" {
    run "./dsh" <<EOF
nonexistentcommand
EOF
stripped_output=$(echo "$output" | tr -d '[:space:]')
    # Expected output should indicate the command was not found
    expected_output="Error executing command: nonexistentcommand"

    # Assertions
    [ "$status" -ne 0 ]
    echo "$output" | grep -q "$expected_output"  # Check if error message is in output
}