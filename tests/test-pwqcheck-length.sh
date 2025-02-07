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

# Test helper function with improved output handling
test_password() {
	local password="$1"
	local min_params="$2"
	local expected="$3"
	local test_name="$4"

    printf "Testing %-40s" "$test_name:"

	PWQCHECK_BIN="$(dirname "$0")/../pwqcheck"

	output=$(echo "$password" | "$PWQCHECK_BIN" -1 "min=$min_params" 2>&1)
	status=$?

	if [ "$expected" = "pass" -a $status -eq 0 ] || \
	   [ "$expected" = "fail" -a $status -ne 0 ]; then
		echo -e "${GREEN}PASS${NC}"
		echo "  Password tested: '$password'"
		echo "  Result: $output"
		echo
	else
		echo -e "${RED}FAIL${NC}"
		echo "  Password tested: '$password'"
		echo "  Expected: $expected"
		echo "  Got: $output"
		echo "  Exit status: $status"
		echo
		return 1
	fi
}

# Main test suite
main() {
	echo "=== Password Length Tests ==="
	echo

	# Test 1: Default minimum lengths
	test_password "short" "24,12,8,7,6" "fail" "Short password"
	test_password "ThisIsAVeryLongPasswordThatShouldPass123!" "24,12,8,7,6" "pass" "Long complex password"

	# Test 2: Custom minimum lengths
	test_password "pass123" "6,6,6,6,6" "pass" "Password with relaxed mins"
	test_password "a" "6,6,6,6,6" "fail" "Single character password"

	# Test 3: Different complexity levels
	test_password "BearD&Tach" "8,8,8,8,8" "pass" "Simple but long password"
	test_password "P@ssw0rd!" "8,8,8,8,8" "pass" "Complex password"

	# Test 4: Edge cases
	test_password "YakMeas1" "8,8,8,8,8" "pass" "Exactly minimum length"
	test_password "7chars" "8,8,8,8,8" "fail" "Below minimum length"

	# Test 5: Different complexity classes
	echo "Testing complexity classes..."
	test_password "FigRatMatBatSatWatPatCat" "24,12,8,7,6" "pass" "N0: 24-char basic password"
	test_password "Complex12Pass" "24,12,8,7,6" "pass" "N1: 12-char mixed password"
	test_password "P@ss8chr" "24,12,8,7,6" "pass" "N2: 8-char complex password"
	test_password "P@s7chr" "24,12,8,7,6" "pass" "N3: 7-char complex password"
	test_password "B!rd5#K" "24,12,8,7,6" "pass" "N4: 6-char highly complex password"
}

# Run the tests
main
