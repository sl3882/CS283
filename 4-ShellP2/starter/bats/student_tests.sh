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



    # Check if the output contains an error message
    [[ "$output" == *"Failed to execute command"* ]] || [[ "$output" == *"execvp"* ]]
}