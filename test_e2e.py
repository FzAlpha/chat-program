import subprocess
import time
import os
import signal
import sys

def run_test():
    port = 9099
    print(f"[TEST] Starting server on port {port}...")
    server_proc = subprocess.Popen(
        ["./bin/server", str(port)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    time.sleep(0.5)

    print("[TEST] Starting Alice client...")
    alice_proc = subprocess.Popen(
        ["./bin/client", "127.0.0.1", str(port), "Alice"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    time.sleep(0.5)

    print("[TEST] Starting Bob client...")
    bob_proc = subprocess.Popen(
        ["./bin/client", "127.0.0.1", str(port), "Bob"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    time.sleep(0.5)

    # Alice sends message
    print("[TEST] Alice sends message: 'Hello Bob!'")
    alice_proc.stdin.write("Hello Bob!\n")
    alice_proc.stdin.flush()
    time.sleep(0.5)

    # Bob sends message
    print("[TEST] Bob sends message: 'Hey Alice!'")
    bob_proc.stdin.write("Hey Alice!\n")
    bob_proc.stdin.flush()
    time.sleep(0.5)

    # Alice queries user list
    print("[TEST] Alice requests /list")
    alice_proc.stdin.write("/list\n")
    alice_proc.stdin.flush()
    time.sleep(0.5)

    # Both quit
    print("[TEST] Quitting clients...")
    alice_proc.stdin.write("/quit\n")
    alice_proc.stdin.flush()
    bob_proc.stdin.write("/quit\n")
    bob_proc.stdin.flush()

    alice_out, alice_err = alice_proc.communicate(timeout=3)
    bob_out, bob_err = bob_proc.communicate(timeout=3)

    # Terminate server cleanly
    server_proc.send_signal(signal.SIGINT)
    server_out, server_err = server_proc.communicate(timeout=3)

    print("\n--- Alice stdout ---")
    print(alice_out)
    print("\n--- Bob stdout ---")
    print(bob_out)
    print("\n--- Server stdout ---")
    print(server_out)

    # Verifications
    assert "Hey Alice!" in alice_out, "Alice did not receive Bob's message!"
    assert "Hello Bob!" in bob_out, "Bob did not receive Alice's message!"
    assert "Online Users" in alice_out, "Alice did not receive online user list!"
    print("\n✅ All automated verification checks PASSED!")

if __name__ == "__main__":
    run_test()
