from flask import Flask
import os

app = Flask(__name__)

@app.route("/shutdown", methods=["GET"])
def shutdown():
    os.system("sudo /sbin/poweroff")
    return "Odroid shutting down..."

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)  # Nasłuchuj na wszystkich interfejsach, port 5000 dla  Flask