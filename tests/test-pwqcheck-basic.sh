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

# Test function for basic password checks
test_basic_password() {
	local password="$1"
	local expected="$2"
	local description="$3"

	printf "%-40s" "$description"

	result=$(echo "$password" | "$PWQCHECK_BIN" -1 2>&1)
	exit_code=$?

	if [ "$expected" = "pass" -a "$exit_code" -eq 0 ] || \
	   [ "$expected" = "fail" -a "$exit_code" -ne 0 ]; then
		echo -e "${GREEN}PASS${NC}"
#		echo "  Password tested: '$password'"
#		echo "  Result: $result"
#		echo
		return 0
	else
		echo -e "${RED}FAIL${NC}"
		echo "Password tested: $password"
		echo "Expected: $expected"
		echo "Got: $result"
		echo
		exit 1
	fi
}

echo "Running Basic Password Validation Tests..."

# Test Suite 1: Strong Passwords
echo -e "\nTesting Strong Passwords:"
test_basic_password "P@ssw0rd123!" "pass" "Standard strong password"
test_basic_password "Tr0ub4dor&3" "pass" "Complex password"
test_basic_password "iStayInloreAtHomeb7&street111" "pass" "Long passphrase"
test_basic_password "H3llo@W0rld2024" "pass" "Strong password with year"

# Test Suite 2: Common Weak Patterns
echo -e "\nTesting Weak Patterns:"
test_basic_password "password123" "fail" "Common password with numbers"
test_basic_password "qwerty" "fail" "Keyboard pattern"
test_basic_password "admin123" "fail" "Common admin password"
test_basic_password "letmein" "fail" "Common weak password"

# Test Suite 3: Mixed Complexity
echo -e "\nTesting Mixed Complexity:"
test_basic_password "MyP@ssw0rd" "pass" "Mixed case with symbols and numbers"
test_basic_password "Str0ng!P@ssphrase" "pass" "Strong with multiple special chars"
test_basic_password "C0mpl3x1ty!" "pass" "Complex but reasonable length"

# Test Suite 4: Edge Cases
echo -e "\nTesting Edge Cases:"
test_basic_password " " "fail" "Single space"
test_basic_password "" "fail" "Empty password"
test_basic_password "$(printf 'a%.0s' {1..71})" "fail" "Very long password"
test_basic_password "ljy8zk9aBJ3hA3TXAAMAQe61ytFohJM4SuPFbA4L1xDqV2JDE1n8BCnLN96evcJMWyTkr9y3" "pass" "Max length password"
test_basic_password "ljy8zk9aBJ3hA3TXAAMAQe61ytFohJM4SuPFbA4L1xDqV2JDE1n8BCnLN96evcJMWyTkr9y312345" "fail" "Max length exceed password"
test_basic_password "is4a4phrase" "pass" "A minimal passphrase" # if this is accepted ...
test_basic_password "is4a4phra5e" "pass" "Passphrase with leetspeak in a word" # ... then so should be this

echo -e "\nBasic password validation tests completed\n"
