#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}


@test "Check if ls lists files" {
    run ./dsh <<EOF                
ls
EOF

    [ "$status" -eq 0 ]
}


@test "Check if exit terminates shell" {
    run ./dsh <<EOF
exit
EOF

    [ "$status" -eq 0 ]
}

@test "Check if ls with invalid option fails" {
    run ./dsh <<EOF
ls --invalid-option
EOF

    # Strip all whitespace (spaces, tabs, newlines) from the output
    stripped_output=$(echo "$output" | tr -d '\t\n\r\f\v")

    # Expected output with all whitespace removed for easier matching
    expected_output="Failed to execute commanddsh2> dsh2> cmd loop returned 0"

    # These echo commands will help with debugging and will only print
    # if the test fails
    echo "Captured stdout:" 
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"

    # Check exact match
    [ "$stripped_output" = "$expected_output" ]
}