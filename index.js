const { spawn } = require("child_process");
const path = require("path");

function monitorDesktop(callback) {
  const exePath = path.join(
    __dirname,
    "bin",
    "Release",
    "isdesktop-visible.exe"
  );
  const proc = spawn(exePath);

  proc.stdout.setEncoding("utf8");
  proc.stdout.on("data", (data) => {
    const lines = data.split(/\r?\n/).filter(Boolean);
    for (const line of lines) {
      if (line.includes("Desktop state changed")) {
        if (line.includes("SHOWN")) {
          callback("visible", line);
        } else if (line.includes("HIDDEN")) {
          callback("hidden", line);
        }
      }
    }
  });

  proc.stderr.on("data", (data) => {
    console.error("[isdesktop-visible error]", data.toString());
  });

  proc.on("exit", (code) => {
    console.log(`[isdesktop-visible exited with code ${code}]`);
  });

  return proc; 
}

module.exports = { monitorDesktop };
