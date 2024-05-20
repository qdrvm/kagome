import subprocess
import sys
import time

def run_test(test_command):
    try:
        result = subprocess.run(test_command, shell=True, check=True)
        return result.returncode
    except subprocess.CalledProcessError as e:
        return e.returncode
def print_with_flush(message):
    print(message)
    sys.stdout.flush()

def main():
    if len(sys.argv) != 2:
        print_with_flush("Usage: python test_runner.py <test_command>")
        sys.exit(1)

    test_command = sys.argv[1]

    print_with_flush(f"Running test: {test_command} (warm-up)")
    first_attempt = run_test(test_command)

    if first_attempt == 0:
        print_with_flush("Test passed on the first attempt (warm-up).")
        sys.exit(0)

    print_with_flush("Warm-up test failed, running test again...")
    time.sleep(10)

    print_with_flush(f"Running test: {test_command} (second attempt)")
    second_attempt = run_test(test_command)

    if second_attempt == 0:
        print_with_flush("Test passed on the second attempt.")
        sys.exit(0)
    else:
        print_with_flush("Test failed on the second attempt.")
        sys.exit(1)

if __name__ == "__main__":
    main()