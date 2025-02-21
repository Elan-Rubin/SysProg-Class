#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "00_mixed_quoted_and_unquoted_arguments" {
    run ./dsh <<EOF
echo Test "Multi Word Argument" unquoted
EOF
    [[ "$output" == *"Test Multi Word Argument unquoted"* ]]
    [ "$status" -eq 0 ]
}

@test "01_check_ls_runs_without_errors" {
    run ./dsh <<EOF                
ls
EOF

    [ "$status" -eq 0 ]
}

@test "02_change_directory_no_args" {
    current=$(pwd)

    cd /tmp
    mkdir -p dsh-test

    run "${current}/dsh" <<EOF                
cd
pwd
EOF

    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="/tmpdsh2>dsh2>dsh2>cmdloopreturned0"

    echo "Captured stdout:" 
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"

    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "04_handle_quoted_arguments_with_spaces" {
    run ./dsh <<EOF
echo "  Hello   World  " '  Another   Quote  '
EOF
    # Expect quotes to preserve internal spaces
    [[ "$output" == *"  Hello   World  "* ]]
    [[ "$output" == *"  Another   Quote  "* ]]
    [ "$status" -eq 0 ]
}

@test "05_error_nonexistent_external_command" {
    run ./dsh <<EOF
nonexistent_foobar_command
exit
EOF
    # Verify error message and non-zero status
    [[ "$output" == *"execvp failed"* ]]
    [ "$status" -ne 0 ]
}

@test "06_whitespace_input_triggers_warning" {
    run ./dsh <<EOF
     
    # Whitespace-only input
exit
EOF
    # Should show "No commands parsed" warning
    [[ "$output" == *"No commands parsed"* ]]
    [ "$status" -eq 0 ]
}

@test "07_mixed_quoted_and_unquoted_arguments" {
    run ./dsh <<EOF
echo Test "Multi Word Argument" unquoted
EOF
    # Verify argument parsing:
    # ["echo", "Test", "Multi Word Argument", "unquoted"]
    [[ "$output" == *"Test Multi Word Argument unquoted"* ]]
    [ "$status" -eq 0 ]
}