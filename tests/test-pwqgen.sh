#!/bin/sh
#
# Copyright (c) 2025 by Zaiba Sanglikar.  See LICENSE.
# This script tests the password generation utility pwqgen for
# basic password generation , uniqueness, error handling ,password entropy
# and config file parsing .


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

# Test function for password generation
test_password_generation() {
	params="$1"
	expected="$2"
	description="$3"

	PWQGEN_BIN="$(dirname "$0")/../pwqgen"

	printf "Testing %s: " "$description"q
	if [ -z "$params" ]; then
		result=$("$PWQGEN_BIN" 2>&1)
	else
		result=$("$PWQGEN_BIN" $params 2>&1)
	fi
	exit_code=$?

	if [ "$expected" = "pass" -a "$exit_code" -eq 0 ] || \
	[ "$expected" = "fail" -a "$exit_code" -ne 0 ]; then
		printf "%sPASS%s\n" "$GREEN" "$NC"
		printf "  Parameters: %s\n" "${params:-'(none)'}"
		printf "  Result: %s\n\n" "$result"
		return 0
	else
		printf "%sFAIL%s\n" "$RED" "$NC"
		printf "  Parameters: %s\n" "${params:-'(none)'}"
		printf "  Expected: %s\n" "$expected"
		printf "  Got exit code: %d\n" "$exit_code"
		printf "  Output: %s\n\n" "$result"
		return 1
	fi
}

# Test function for password uniqueness
test_password_uniqueness() {
	params="$1"
	description="$2"

	PWQGEN_BIN="$(dirname "$0")/../pwqgen"

	printf "Testing %s: " "$description"
	pass1=$("$PWQGEN_BIN" $params 2>&1)
	exit_code1=$?
	pass2=$("$PWQGEN_BIN" $params 2>&1)
	exit_code2=$?

	if [ $exit_code1 -eq 0 ] && [ $exit_code2 -eq 0 ] && [ "$pass1" != "$pass2" ]; then
		printf "%sPASS%s\n" "$GREEN" "$NC"
		printf "  Parameters: %s\n" "${params:-'(none)'}"
		printf "  Password 1: %s\n" "$pass1"
		printf "  Password 2: %s\n\n" "$pass2"
		return 0
	else
		printf "%sFAIL%s\n" "$RED" "$NC"
		printf "  Parameters: %s\n" "${params:-'(none)'}"
		printf "  Passwords are identical or generation failed\n"
		printf "  Password 1: %s\n" "$pass1"
		printf "  Password 2: %s\n\n" "$pass2"
		return 1
	fi
}

# Function to test passphrase uniqueness for multiple iterations
test_uniqueness() {
	i=0
	iterations=30
	duplicates=0

	while [ $i -lt $iterations ]; do
		pass=$($PWQGEN_BIN)

		# Store passwords in a temporary file
		if [ $i -eq 0 ]; then
		echo "$pass" > temp_passes.txt
		else
		# Check for duplicates
		if grep -Fxq "$pass" temp_passes.txt; then
			duplicates=$((duplicates + 1))
		fi
		echo "$pass" >> temp_passes.txt
		fi

		i=$((i + 1))
	done

	rm -f temp_passes.txt

	#check for the duplicate count
	if [ $duplicates -le 1 ]; then
		return 0
	else
		return 1
	fi

}

# Function to test if password contains different character classes
test_password_entropy() {
	params="$1"
	description="$2"

	printf "Testing %s: " "$description"
	PWQGEN_BIN="$(dirname "$0")/../pwqgen"
	password=$("$PWQGEN_BIN" $params 2>&1)
	exit_code=$?

	# Check if the password has different character classes
	has_lower=$(printf "%s" "$password" | grep -q '[a-z]'; echo $?)
	has_upper=$(printf "%s" "$password" | grep -q '[A-Z]'; echo $?)
	has_digit=$(printf "%s" "$password" | grep -q '[0-9]'; echo $?)
	has_special=$(printf "%s" "$password" | grep -q '[^a-zA-Z0-9]'; echo $?)

	if [ "$has_lower" -eq 0 ] && [ "$has_upper" -eq 0 ] && \
	[ "$has_digit" -eq 0 ] && [ "$has_special" -eq 0 ]; then
		printf "%sPASS%s\n" "$GREEN" "$NC"
		printf "  Parameters: %s\n" "${params:-'(none)'}"
		printf "  Password: %s\n\n" "$password"
		return 0
	else
		printf "%sFAIL%s\n" "$RED" "$NC"
		printf "  Parameters: %s\n" "${params:-'(none)'}"
		printf "  Password doesn't contain all required character classes\n"
		printf "  Password: %s\n\n" "$password"
		return 1
	fi
}

printf "Running Basic Password Generation Tests...\n"

# Test Suite 1: Basic Generation
printf "\nTesting Basic Password Generation:\n"
test_password_generation "" "pass" "Default password generation"
test_password_generation "random=64" "pass" "64-bit random password"
test_password_generation "random=128" "pass" "128-bit random password"

# Test Suite 2: Password Uniqueness
printf "\nTesting Password Uniqueness:\n"
test_password_uniqueness "" "Default password uniqueness"
test_password_uniqueness "random=64" "64-bit random password uniqueness"
test_password_uniqueness "random=128" "128-bit random password uniqueness"

# Test Suite 3: Invalid Parameters
printf "\nTesting Invalid Parameters:\n"
test_password_generation "random=-1" "fail" "Negative random bits"
test_password_generation "random=0" "fail" "Zero random bits"
test_password_generation "random=999999" "fail" "Excessive random bits"
test_password_generation "random=invalid" "fail" "Invalid random value"
test_password_generation "random=abc" "fail" "Non-numeric random bits"
test_password_generation "random=" "fail" "Empty random value"
test_password_generation "random=1.5" "fail" "Decimal random bits"

# Test Suite 4: Multiple Parameters
printf "\nTesting Multiple Parameters:\n"
test_password_generation "random=64 max=40" "pass" "Multiple valid parameters"
test_password_generation "random=64 invalid=parameter" "fail" "Valid and invalid parameters"

# Test Suite 5: Password uniqueness over multiple iterations
printf "\nPassword uniqueness over multiple iterations\n"
if test_uniqueness; then
	printf "%sPASS%s\n" "$GREEN" "$NC"
else
	printf "%sFAIL%s\n" "$RED" "$NC"
	printf "Too many duplicate passphrases generated\n"
fi

# Test Suite 6: Test configuration files
printf "\nTesting config files:\n"

tmp_dir=$(mktemp -d)
trap 'rm -rf "${tmp_dir}"' EXIT

# Valid config file
cat > "${tmp_dir}/valid.conf" << EOF
min=disabled,24,11,8,7
max=72
passphrase=3
match=4
similar=deny
random=47
enforce=everyone
retry=3
EOF

# Invalid config file
cat > "${tmp_dir}/invalid.conf" << EOF
min=invalid
max=abc
passphrase=0000
match=ccc
similar=1
random=0
EOF

#Empty config file
cat > "${tmp_dir}/empty.conf" << EOF
EOF

test_password_generation "config=${tmp_dir}/valid.conf" "pass" "Valid configuration file"
test_password_generation "config=${tmp_dir}/invalid.conf" "fail" "Invalid configuration file"
test_password_generation "config=${tmp_dir}/empty.conf" "pass" "Empty configuration file"

# Test Suite 7: Test password entropy
test_password_entropy "random=128" "Password should contain various character classes"