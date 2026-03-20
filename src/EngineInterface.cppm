module;
#include <windows.h>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <iostream>
#include <vector>
export module EngineInterface;

namespace EngineInterface {

    export class EngineBridge {
    private:
        HANDLE hProcess_ = NULL;
        HANDLE hThread_ = NULL;
        HANDLE hStdinWrite_ = NULL;
        HANDLE hStdoutRead_ = NULL;
        std::thread listenerThread_;
        std::atomic<bool> isRunning_{ false };
        std::mutex dataMutex_;

        int currentDepth_ = 0;
        int currentEval_ = 0;
        std::string bestMove_;

        void listenerLoop() {
            char buffer[4096];
            DWORD bytesRead;
            std::string leftovers;

            while (isRunning_ && ReadFile(hStdoutRead_, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string chunk = leftovers + buffer;
                size_t pos;
                while ((pos = chunk.find('\n')) != std::string::npos) {
                    std::string line = chunk.substr(0, pos);
                    processLine(line);
                    chunk.erase(0, pos + 1);
                }
                leftovers = chunk;
            }
        }

        void processLine(const std::string& line) {
            std::lock_guard<std::mutex> lock(dataMutex_);

            if (line.starts_with("bestmove ")) {
                std::istringstream iss(line.substr(9));
                iss >> bestMove_;
                return;
            }

            if (!line.starts_with("info ")) return;

            std::istringstream iss(line.substr(5));
            std::string token;
            int newDepth = -1;
            bool foundScore = false;
            int scoreValue = 0;
            bool isMate = false;

            while (iss >> token) {
                if (token == "depth") {
                    iss >> newDepth;
                }
                else if (token == "score") {
                    std::string type;
                    iss >> type;
                    if (type == "cp") {
                        iss >> scoreValue;
                        isMate = false;
                        foundScore = true;
                    }
                    else if (type == "mate") {
                        iss >> scoreValue;
                        isMate = true;
                        foundScore = true;
                    }
                }
            }

            if (foundScore && (newDepth > currentDepth_ || currentDepth_ <= 0)) {
                currentDepth_ = newDepth;
                if (isMate) {
                    currentEval_ = 30000 + scoreValue;
                }
                else {
                    currentEval_ = scoreValue;
                }
            }
        }

    public:
        explicit EngineBridge(const std::wstring& enginePath) {
            SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };

            HANDLE hStdinRead{}, hStdoutWrite{};
            CreatePipe(&hStdoutRead_, &hStdoutWrite, &sa, 0);
            CreatePipe(&hStdinRead, &hStdinWrite_, &sa, 0);

            SetHandleInformation(hStdoutRead_, HANDLE_FLAG_INHERIT, 0);
            SetHandleInformation(hStdinWrite_, HANDLE_FLAG_INHERIT, 0);

            STARTUPINFOW si{};
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdOutput = hStdoutWrite;
            si.hStdError = hStdoutWrite;
            si.hStdInput = hStdinRead;

            PROCESS_INFORMATION pi{};
            wchar_t cmd[MAX_PATH];
            wcscpy_s(cmd, enginePath.c_str());

            if (!CreateProcessW(nullptr, cmd, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
                std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
                return;
            }

            hProcess_ = pi.hProcess;
            hThread_ = pi.hThread;

            CloseHandle(hStdoutWrite);
            CloseHandle(hStdinRead);

            isRunning_ = true;
            listenerThread_ = std::thread(&EngineBridge::listenerLoop, this);

            sendCommand("uci");
            sendCommand("isready");
        }

        ~EngineBridge() {
            isRunning_ = false;

            if (isRunning_) {
                sendCommand("quit");
                CloseHandle(hStdinWrite_);

                if (hProcess_) {
                    if (WaitForSingleObject(hProcess_, 3000) == WAIT_TIMEOUT) {
                        TerminateProcess(hProcess_, 1);
                    }
                    CloseHandle(hProcess_);
                    CloseHandle(hThread_);
                }
            }

            if (listenerThread_.joinable()) {
                CloseHandle(hStdoutRead_);
                listenerThread_.join();
            }
        }

        void sendCommand(const std::string& cmd) {
            if (!isRunning_ || !hStdinWrite_) return;
            std::string full = cmd + "\n";
            DWORD written = 0;
            if (!WriteFile(hStdinWrite_, full.data(), static_cast<DWORD>(full.size()), &written, nullptr)) {
                std::cerr << "Failed to send command: " << cmd << " Error: " << GetLastError() << '\n';
            }
        }

        void newGame() { sendCommand("ucinewgame"); }
        void setPositionStartpos(const std::vector<std::string>& moves = {}) {
            std::string cmd = "position startpos";
            if (!moves.empty()) {
                cmd += " moves";
                for (const auto& m : moves) cmd += " " + m;
            }
            sendCommand(cmd);
        }

        void goDepth(int plies) { sendCommand("go depth " + std::to_string(plies)); }
        void goMovetime(int ms) { sendCommand("go movetime " + std::to_string(ms)); }
        void goInfinite() { sendCommand("go infinite"); }
        void stop() { sendCommand("stop"); }

        int  getDepth(){
            std::lock_guard<std::mutex> lock(dataMutex_);
            return currentDepth_;
        }

        int  getEval(){
            std::lock_guard<std::mutex> lock(dataMutex_);
            return currentEval_;
        }

        bool isMateScore(){
            std::lock_guard<std::mutex> lock(dataMutex_);
            return currentEval_ >= 30000 || currentEval_ <= -30000;
        }

        int  getMateInPlies(){
            std::lock_guard<std::mutex> lock(dataMutex_);
            if (currentEval_ >= 30000) return currentEval_ - 30000;
            if (currentEval_ <= -30000) return -(std::abs(currentEval_) - 30000);
            return 0;
        }

        std::string consumeBestMove() {
            std::lock_guard<std::mutex> lock(dataMutex_);
            std::string m = std::move(bestMove_);
            bestMove_.clear();
            return m;
        }

        std::string peekBestMove(){
            std::lock_guard<std::mutex> lock(dataMutex_);
            return bestMove_;
        }
    };

}