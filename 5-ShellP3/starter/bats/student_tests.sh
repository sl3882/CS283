#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file




@test "Single Pipe Redirection" {
    run "./dsh" <<EOF
echo Hello | wc -c
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="6dsh3>dsh3>cmdloopreturned0"
    # These echo commands will help with debugging and will only print
    # if the test fails
    echo "Captured stdout:" 
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    # Check exact match
    [ "$stripped_output" = "$expected_output" ]
    # Assertions
    [ "$status" -eq 0 ]
}

@test "Built-in Command - cd" {
    run "./dsh" <<EOF
cd /tmp
pwd
EOF
    # We expect the pwd to show /tmp directory
    echo "Captured stdout:" 
    echo "Output: $output"
    echo "Exit Status: $status"
    # Check that command executed successfully
    [ "$status" -eq 0 ]
    # Check that output contains /tmp
    [[ "$output" == *"/tmp"* ]]
}

@test "Exit Command" {
    run "./dsh" <<EOF
exit
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="dsh3>cmdloopreturned0"
    
    echo "Captured stdout:" 
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    
    # Check exact match
    [ "$stripped_output" = "$expected_output" ]
    # Assertions
    [ "$status" -eq 0 ]
}

@test "Multiple Pipelines" {
    run "./dsh" <<EOF
ls | cat dshlib.h | wc -l
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="92dsh3>dsh3>cmdloopreturned0"

    # These echo commands will help with debugging and will only print
    #if the test fails
    echo "Captured stdout:" 
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"

    # Check exact match
    [ "$stripped_output" = "$expected_output" ]

    # Assertions
    [ "$status" -eq 0 ]
}
@test "Basic Command - echo" {
    run "./dsh" <<EOF
echo Hello,World!
EOF
stripped_output=$(echo "$output" | tr -d '[:space:]')
    # Expected output
    expected_output="Hello,World!dsh3>dsh3>cmdloopreturned0"

    # These echo commands will help with debugging and will only print
    #if the test fails
    echo "Captured stdout:" 
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"

    # Check exact match
    [ "$stripped_output" = "$expected_output" ]

    # Assertions
    [ "$status" -eq 0 ]
}
@test "Invalid Command" {
    run "./dsh" <<EOF
nonexistentcommand
EOF
stripped_output=$(echo "$output" | tr -d '[:space:]')
    # Expected output should indicate the command was not found
    expected_output="execvp:Nosuchfileordirectorydsh3>dsh3>Errorexecutingcommand:nonexistentcommanddsh3>cmdloopreturned0"

    # These echo commands will help with debugging and will only print
    #if the test fails
    echo "Captured stdout:" 
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"

    # Check exact match
    [ "$stripped_output" = "$expected_output" ]

    # Assertions
    [ "$status" -eq 0 ]
}