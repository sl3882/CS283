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



@test "Check if unknown command fails" {
    run ./dsh <<EOF                
invalid_command
EOF

    [ "$status" -ne 0 ]
}