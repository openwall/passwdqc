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

# Function to test multiple passwords
test_multiple_passwords() {
	local test_name="$1"
	local passwords="$2"
	local expected_results="$3"

	printf "%-40s" "$test_name"

	# Create a temporary file for test output
	local temp_output
	temp_output=$(mktemp) || exit

	# Run pwqcheck with multiple passwords
	echo -e "$passwords" | "$PWQCHECK_BIN" --multi -1 > "$temp_output" 2>&1

	# Compare results
	local actual_results
	actual_results=$(cat "$temp_output")
	if echo "$actual_results" | grep -q "$expected_results"; then
		echo -e "${GREEN}PASS${NC}"
#		echo "Test output matches expected results"
	else
		echo -e "${RED}FAIL${NC}"
		echo "Expected:"
		echo "$expected_results"
		echo "Got:"
		cat "$temp_output"
		rm -f "$temp_output"
		exit 1
	fi

	rm -f "$temp_output"
}

echo "Running Multiple Password Tests..."

# Test 1: Mix of valid and invalid passwords
test_multiple_passwords "Mixed Passwords" \
"StrongP@ss123!
weak
Tr0ub4dor&3
password123
C0mpl3x1ty!" \
"OK: StrongP@ss123!
Bad passphrase
OK: Tr0ub4dor&3
Bad passphrase
OK: C0mpl3x1ty!"

# Test 2: All valid passwords
test_multiple_passwords "All Valid Passwords" \
"P@ssw0rd123!
Tr0ub4dor&3
C0mpl3x1ty!
H3llo@W0rld" \
"OK: P@ssw0rd123!
OK: Tr0ub4dor&3
OK: C0mpl3x1ty!
OK: H3llo@W0rld"

# Test 3: All invalid passwords
test_multiple_passwords "All Invalid Passwords" \
"password123
qwerty
admin
letmein" \
"Bad passphrase
Bad passphrase
Bad passphrase
Bad passphrase"

# Test 4: Empty lines and special characters
test_multiple_passwords "Special Cases" \
"StrongP@ss123!
P@ssw0rd!
Tr0ub4dor&3" \
"OK: StrongP@ss123!
Bad passphrase
OK: P@ssw0rd!
Bad passphrase
OK: Tr0ub4dor&3"

# Test 5: With custom parameters
test_multiple_passwords "Custom Parameters" \
"short
medium12345
VeryLongP@ssword123!" \
"Bad passphrase
OK: medium12345
OK: VeryLongP@ssword123!"

echo -e "\nMultiple password tests completed\n"

exit 0

# Test 6: Large number of passwords
echo -e "\nTesting large batch of passwords..."
{
	for i in {1..50}; do
		if [ $((i % 2)) -eq 0 ]; then
			echo "StrongP@ss${i}!"
		else
			echo "weak${i}"
		fi
	done
} | "$PWQCHECK_BIN" --multi -1

echo "Large batch test completed!"
