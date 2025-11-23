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

# Test helper function with improved output handling
test_password() {
	local password="$1"
	local min_params="$2"
	local expected="$3"
	local test_name="$4"

	printf "%-40s" "$test_name"

	output=$(echo "$password" | "$PWQCHECK_BIN" -1 "min=$min_params" 2>&1)
	status=$?

	if [ "$expected" = "pass" -a $status -eq 0 ] || \
	   [ "$expected" = "fail" -a $status -ne 0 ]; then
		echo -e "${GREEN}PASS${NC}"
#		echo "  Password tested: '$password'"
#		echo "  Result: $output"
#		echo
	else
		echo -e "${RED}FAIL${NC}"
		echo "  Password tested: '$password'"
		echo "  Expected: $expected"
		echo "  Got: $output"
		echo "  Exit status: $status"
		echo
		exit 1
	fi
}

echo "Password Length Tests"
echo

# Test 1: Default minimum lengths
test_password "short" "24,12,8,7,6" "fail" "Short password"
test_password "ThisIsAVeryLongPasswordThatShouldPass123!" "24,12,8,7,6" "pass" "Long complex password"

# Test 2: Custom minimum lengths
test_password "rare123" "6,6,6,6,6" "pass" "Password with relaxed mins"
test_password "a" "6,6,6,6,6" "fail" "Single character password"

# Test 3: Different complexity levels
test_password "BearD&Tach" "8,8,8,8,8" "pass" "Simple but long password"
test_password "Ar4rew0rd!" "8,8,8,8,8" "pass" "Complex password"

# Test 4: Edge cases
test_password "YakMeas1" "8,8,8,8,8" "pass" "Exactly minimum length"
test_password "7 chars" "8,8,8,8,8" "fail" "Below minimum length"

# Test 5: Different complexity classes
echo "Testing complexity classes..."
test_password "rkshnwkuvsgisjbybsifyvubaxukqizqpxyc" "36,24,11,8,7" "pass" "N0: 36-char 1-class random"
test_password "figratmatbatsatwatpatcatgdpjrgvapduc" "36,24,11,8,7" "fail" "N0: 36-char 1-class word-based"
test_password "rkshnwkuvsgisjbybsifyv24" "36,24,11,8,7" "pass" "N1: 24-char 2-class random"
test_password "rkshnwkuvsgisjbybsifyvu4" "36,24,11,8,7" "fail" "N1: 24-char effectively 1-class random"
test_password "min one phr" "36,24,11,8,7" "pass" "N2: 11-char 3-word passphrase"
test_password "min onesphr" "36,24,11,8,7" "fail" "N2: 11-char 2-word non-passphrase"
test_password "m@s58chr" "36,24,11,8,7" "pass" "N3: 8-char barely complex"
test_password "mas58chr" "36,24,11,8,7" "fail" "N3: 8-char insufficiently complex"
test_password "B!re5#K" "36,24,11,8,7" "pass" "N4: 7-char highly complex"
test_password "B1rd5#K" "36,24,11,8,7" "fail" "N4: 7-char insufficiently complex"

echo -e "\nPassword length tests completed\n"
