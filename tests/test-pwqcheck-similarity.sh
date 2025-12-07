#!/bin/bash
#
# Copyright (c) 2025 by Zaiba Sanglikar.  See LICENSE.
#
#

set -o pipefail

PWQCHECK_BIN="$(dirname "$0")/../pwqcheck"

if [ -t 1 ]; then
# Colors for better visibility
	GREEN='\033[0;32m'
	RED='\033[0;31m'
	NC='\033[0m'
else
	GREEN=''
	RED=''
	NC=''
fi

# Function to run pwqcheck with two passwords
test_passwords() {
	local new_pass="$1"
	local old_pass="$2"
	local expected_result="$3"
	local test_name="$4"

	printf "%-40s" "$test_name"

	# Run pwqcheck with both passwords and similar=deny option
	result=$(printf "%s\n%s" "$new_pass" "$old_pass" | "$PWQCHECK_BIN" -2 similar=deny 2>&1)
	exit_code=$?

	# Check if the result matches expected
	if [ $exit_code -eq "$expected_result" ]; then
		echo -e "${GREEN}PASS${NC}"
	else
		echo -e "${RED}FAIL${NC}"
		echo "  Expected exit code: $expected_result, Got: $exit_code"
		echo "  Output: $result"
		exit 1
	fi

}

# Function to test when similar passwords are permitted
test_passwords_permit() {
	local new_pass="$1"
	local old_pass="$2"
	local expected_result="$3"
	local test_name="$4"

	printf "%-40s" "$test_name"

	# Run pwqcheck with both passwords and similar=permit option
	result=$(printf "%s\n%s" "$new_pass" "$old_pass" | "$PWQCHECK_BIN" -2 similar=permit 2>&1)
	exit_code=$?

	# Check if the result matches expected
	if [ $exit_code -eq "$expected_result" ]; then
		echo -e "${GREEN}PASS${NC}"
	else
		echo -e "${RED}FAIL${NC}"
		echo "  Expected exit code: $expected_result, Got: $exit_code"
		echo "  Output: $result"
		exit 1
	fi
}

# Main testing section
echo "Running pwqcheck similarity tests with similar=deny..."

# Test 1: Identical passwords (should fail with deny)
test_passwords "ComplexIdentical3!" "ComplexIdentical3!" 1 \
    "Identical passwords rejected"

# Test 2: Case variation (should fail with deny)
test_passwords "ComplexIdentical3!" "complexidentical3!" 1 \
    "Case variations rejected"

# Test 3: Number substitution (should fail with deny)
test_passwords "ComplexIdentical3!" "C0mpl3xId3ntic4l3" 1 \
    "Common number substitutions rejected"

# Test 4: Different passwords (should pass even with deny)
test_passwords "TotallyDifferent456@" "OriginalWas123!" 0 \
    "Different passwords accepted"

echo
echo "Running pwqcheck similarity tests with similar=permit..."

# Test 5: Identical passwords (should fail even with permit)
test_passwords_permit "ComplexIdentical3!" "ComplexIdentical3!" 1 \
    "Identical passwords accepted"

# Test 6: Case variation (should pass with permit)
test_passwords_permit "ComplexIdentical3!" "complexidentical3!" 0 \
    "Case variations accepted"

# Test 7: Number substitution (should pass with permit)
test_passwords_permit "ComplexIdentical3!" "C0mpl3xId3ntic4l3" 0 \
    "Common number substitutions accepted"

# Test 8: Different passwords (should pass with permit)
test_passwords_permit "TotallyDifferent456@" "OriginalWas123!" 0 \
    "Different passwords accepted"

echo -e "\npwqcheck similarity tests completed\n"
