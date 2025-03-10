#!/bin/sh
#
# Copyright (c) 2025 by Zaiba Sanglikar.  See LICENSE.
#
# This script tests the password filter utility pwqfilter for
# mode options lookup ,insert,status,create,test-fp-rate

if [ -t 1 ]; then
        # Colors for better visibility
        GREEN="$(printf "\033[0;32m")"
        RED="$(printf "\033[0;31m")"
        NC="$(printf "\033[0m")"
else
        GREEN=''
        RED=''
        NC=''
fi


# Test function for pwqfilter operations
test_pwqfilter_operation() {
	params="$1"
	expected="$2"
	description="$3"
	input="$4"

	PWQFILTER_BIN="$(dirname "$0")/../pwqfilter"

	printf "Testing %s: " "$description"
	if [ -z "$input" ]; then
		result=$($PWQFILTER_BIN $params 2>&1)
	else
		result=$(echo -e "$input" | $PWQFILTER_BIN $params 2>&1)

	fi
	exit_code=$?

	if [ "$expected" = "pass" -a "$exit_code" -eq 0 ] || \
	[ "$expected" = "fail" -a "$exit_code" -ne 0 ]; then
		printf "%sPASS%s\n" "$GREEN" "$NC"
		printf "  Result: %s\n\n" "$result"
		return 0
	else
		printf "%sFAIL%s\n" "$RED" "$NC"
		printf "  Parameters: %s\n" "${params:-'(none)'}"
		printf "  Input: %s\n" "${input:-'(none)'}"
		printf "  Expected: %s\n" "$expected"
		printf "  Got exit code: %d\n" "$exit_code"
		printf "  Output: %s\n\n" "$result"
		return 1
	fi
}

# Function to test lookup functionality
test_lookup() {
	filter_file="$1"
	input="$2"
	expected_result="$3"
	description="$4"

	PWQFILTER_BIN="$(dirname "$0")/../pwqfilter"
	printf "Testing %s: " "$description"

	# Perform lookup
	result=$(echo "$input" | $PWQFILTER_BIN --filter="$filter_file" --lookup 2>&1)
	exit_code=$?

	# For "found" we expect exit code 0 and output matching input
	# For "not found" we expect no output (or empty string)
	if [ "$expected_result" = "pass" ] && [ "$exit_code" -eq 0 ] && [ "$result" = "$input" ]; then
		printf "%sPASS%s\n" "$GREEN" "$NC"
		printf "  Input '%s' was found in filter as expected\n\n" "$input"
		printf "  Result: %s\n\n" "$result"
		return 0
	elif [ "$expected_result" = "fail" ] && [ -z "$result" ]; then
		printf "%sPASS%s\n" "$GREEN" "$NC"
		printf "  Input '%s' was not found in filter as expected\n\n" "$input"
		printf "  Result: %s\n\n" "$result"
		return 0
	else
		printf "%sFAIL%s\n" "$RED" "$NC"
		printf "  Filter file: %s\n" "$filter_file"
		printf "  Input: %s\n" "$input"
		printf "  Expected: %s\n" "$expected_result"
		printf "  Got: %s\n" "$result"
		printf "  Exit code: %d\n\n" "$exit_code"
		return 1
	fi
}

# Create temporary directory
tmp_dir=$(mktemp -d)
trap 'rm -rf "${tmp_dir}"' EXIT

# Create password lists for testing
cat > "${tmp_dir}/passwords.txt" << EOF
password123
qwerty
letmein
admin123
welcome
12345678
EOF


printf "\nRunning PWQFilter Tests...\n"
printf "============================="

# Test Suite 1: Basic Mode Options
printf "\nTesting Basic Mode Options:\n"

# Test --create mode
test_pwqfilter_operation "--create=10 --output=${tmp_dir}/basic.filter" "pass" "Create filter with --create" "$(cat ${tmp_dir}/passwords.txt)"

# Test --status mode
test_pwqfilter_operation "--status --filter=${tmp_dir}/basic.filter" "pass" "Check filter status with --status"

# Test --lookup mode (default)
test_pwqfilter_operation "--filter=${tmp_dir}/basic.filter" "pass" "Look up passwords with --lookup (default)" "password123"

# Test nonexistent passwords
test_pwqfilter_operation "--filter=${tmp_dir}/basic.filter" "fail" "Look up nonexistent passwords" "nonexistent123"

# Test --insert mode
test_pwqfilter_operation "--insert --filter=${tmp_dir}/basic.filter --output=${tmp_dir}/updated.filter" "pass" "Insert entries with --insert" "newpassword123"

# Test --test-fp-rate mode
test_pwqfilter_operation "--test-fp-rate --filter=${tmp_dir}/updated.filter" "pass" "Test false positive rate with --test-fp-rate"


# Test Suite 2: Specific Lookup Tests
printf "\nTesting Specific Lookup Functionality:\n"

# Create a filter with known passwords
echo -e "\ntestpassword\nsecret123\nadmin123\nsolid" > "${tmp_dir}/known_passwords.txt"
test_pwqfilter_operation "--create=10 --output=${tmp_dir}/lookup_test.filter" "pass" "Create filter for lookup testing" "$(cat ${tmp_dir}/known_passwords.txt)"

# Test lookup for passwords in the filter
test_lookup "${tmp_dir}/lookup_test.filter" "testpassword" "pass" "Password in filter should be found"
test_lookup "${tmp_dir}/lookup_test.filter" "admin123" "pass" "Password in filter should be found"
test_lookup "${tmp_dir}/lookup_test.filter" "solid" "pass" "Password in filter should be found"

# Test lookup for passwords not in the filter
test_lookup "${tmp_dir}/lookup_test.filter" "unknown123" "fail" "Password not in filter should not be found"
test_lookup "${tmp_dir}/lookup_test.filter" "differentpassword" "fail" "Password not in filter should not be found"

# Test Suite 3: Combined Operations
printf "\nTesting Combined Operations:\n"

# Test create + test-fp-rate
test_pwqfilter_operation "--create=30 --output=${tmp_dir}/combined.filter --test-fp-rate" "pass" "Create filter and test FP rate in one command" "$(cat ${tmp_dir}/passwords.txt)"

# Test lookup + status
test_pwqfilter_operation "--filter=${tmp_dir}/combined.filter --status" "pass" "Look up and check status" "password123"

# Test invalid combinations
test_pwqfilter_operation "--create=10 --insert" "fail" "Invalid combination: create + insert" "test"

# Print summary
printf "\nPWQFilter mode options tests completed.\n"
printf "Check the results above to see which tests passed or failed.\n"