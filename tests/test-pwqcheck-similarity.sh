#!/bin/bash
#
# Copyright (c) 2025 by Zaiba Sanglikar.  See LICENSE.
#
#

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

	PWQCHECK_BIN="$(dirname "$0")/../pwqcheck"

	# Run pwqcheck with both passwords and --similar=deny option
	result=$(printf "%s\n%s" "$new_pass" "$old_pass" |  "$PWQCHECK_BIN"  -2 similar=deny 2>&1)
	exit_code=$?

	# Check if the result matches expected
	if [ $exit_code -eq "$expected_result" ]; then
		echo -e "${GREEN}✓ PASS:${NC} $test_name"
	else
		echo -e "${RED}✗ FAIL:${NC} $test_name"
		echo "  Expected exit code: $expected_result, Got: $exit_code"
		echo "  Output: $result"
	fi

}

#add a function to test when similar passwords are permitted
test_passwords_permit() {

	local new_pass="$1"
	local old_pass="$2"
	local expected_result="$3"
	local test_name="$4"

	# Run pwqcheck with both passwords and --similar=permit option
	result=$(printf "%s\n%s" "$new_pass" "$old_pass" | "$PWQCHECK_BIN"  -2  similar=permit 2>&1)
	exit_code=$?

	# Check if the result matches expected
	if [ $exit_code -eq "$expected_result" ]; then
		echo -e "${GREEN}✓ PASS:${NC} $test_name"
		((TESTS_PASSED++))
	else
		echo -e "${RED}✗ FAIL:${NC} $test_name"
		echo "  Expected exit code: $expected_result, Got: $exit_code"
		echo "  Output: $result"
	fi
}

# Main testing section
echo "Running pwqcheck similarity tests with --similar=deny..."
echo "===================================================="

# Test 1: Identical passwords (should fail with deny)
test_passwords "ComplexPass123!" "ComplexPass123!" 1 \
    "Identical passwords should be rejected when similarity is denied"

# Test 2: Case variation (should fail with deny)
test_passwords "ComplexPass123!" "complexpass123!" 1 \
    "Case variations should be rejected when similarity is denied"

# Test 3: Number substitution (should fail with deny)
test_passwords "P@ssw0rd123!" "Password123!" 1 \
    "Common number substitutions should be rejected when similarity is denied"

# Test 4: Different passwords (should pass even with deny)
test_passwords "ComplexPass123!" "TotallyDifferent456@" 0 \
    "Different passwords should be accepted even when similarity is denied"

echo
echo "Running pwqcheck similarity tests with --similar=permit..."
echo "====================================================="

# Test 5: Identical passwords (should pass with permit)
test_passwords_permit "VeryComplexPass#789" "VeryComplexPass#789!" 0 \
    "Identical passwords should be accepted when similarity is permitted"

# Test 6: Case variation (should pass with permit)
test_passwords_permit "ComplexPass123!" "complexpass123!" 0 \
    "Case variations should be accepted when similarity is permitted"

# Test 7: Number substitution (should pass with permit)
test_passwords_permit "P@ssw0rd123!" "Password123!" 0 \
    "Common number substitutions should be accepted when similarity is permitted"

# Test 8: Different passwords (should pass with permit)
test_passwords_permit "ComplexPass123!" "TotallyDifferent456@" 0 \
    "Different passwords should be accepted when similarity is permitted"