import subprocess
import sys

def run_test(container_name, test_command):
    command = f"docker exec {container_name} sh -c 'zombienet-linux-x64 test -p native {test_command}'"
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    while True:
        output = process.stdout.readline()
        if output == b'' and process.poll() is not None:
            break
        if output:
            print(output.strip().decode())

    rc = process.poll()

    if rc != 0:
        stderr_output = process.stderr.read().decode()
        print(stderr_output)

    return rc

def stop_container(container_name):
    subprocess.run(f"docker stop {container_name}", shell=True)
    subprocess.run(f"docker rm {container_name}", shell=True)

def print_with_flush(message):
    print(message)
    sys.stdout.flush()

def main():
    if len(sys.argv) != 3:
        print_with_flush("Usage: python test_runner.py <image_name> <test_command>")
        sys.exit(1)

    image_name = sys.argv[1]
    test_command = sys.argv[2]

    container_name = "zombie-tester-container"
    print_with_flush("Starting the tester container...")
    subprocess.run(f"docker run -d --name {container_name} {image_name} tail -f /dev/null", shell=True, check=True)
    print_with_flush(f"Container started with name: {container_name}")

    print_with_flush(f"Running test: {test_command} (warm-up)")
    first_attempt = run_test(container_name, test_command)

    if first_attempt == 0:
        print_with_flush("Test passed on the first attempt (warm-up).")
        stop_container(container_name)
        sys.exit(0)

    print_with_flush("Warm-up test failed, running test again...")

    print_with_flush(f"Running test: {test_command} (second attempt)")
    second_attempt = run_test(container_name, test_command)

    if second_attempt == 0:
        print_with_flush("Test passed on the second attempt.")
        stop_container(container_name)
        sys.exit(0)
    else:
        print_with_flush("Test failed on the second attempt.")
        stop_container(container_name)
        sys.exit(1)

if __name__ == "__main__":
    main()
