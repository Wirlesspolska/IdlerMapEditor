const { exec, spawn } = require("child_process");
const psList = require("ps-list");

const EDITOR_PATH = "Editor_x64.exe";

async function findProcess(name) {
    const processes = await psList();
    return processes.find(p => p.name.toLowerCase() === name.toLowerCase());
}

async function suspendProcess(pid) {
    exec(`powershell -Command "Stop-Process -Id ${pid} -Force"`, (error) => {
        if (error) console.log("Error suspending process:", error);
        else console.log("Process suspended.");
    });
}

async function runEditor() {
    spawn(EDITOR_PATH, { detached: true });
    console.log("Second instance launched.");
}

(async () => {
    const processInfo = await findProcess("Editor_x64.exe");
    if (processInfo) {
        console.log(`Editor found with PID ${processInfo.pid}. Suspending.`);
        await suspendProcess(processInfo.pid);
    }
    runEditor();
})();
