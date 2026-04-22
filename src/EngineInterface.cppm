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
    export struct EngineInfo {
        int depth;
        double nps;
    };

    export class EngineInterface {
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
        size_t nps_ = 0;
        std::string bestMove_;
        std::string pvMove_;

        void listener_loop() {
            char buffer[4096];
            DWORD bytesRead;
            std::string leftovers;

            while (isRunning_ && ReadFile(hStdoutRead_, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string chunk = leftovers + buffer;
                size_t pos;
                while ((pos = chunk.find('\n')) != std::string::npos) {
                    std::string line = chunk.substr(0, pos);
                    process_line(line);
                    chunk.erase(0, pos + 1);
                }
                leftovers = chunk;
            }
        }

        void process_line(const std::string& line) {
			std::cout << "[ ENGINE ]" << line << '\n';
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
                else if (token == "nps") {
                    iss >> nps_;
                }
                else if (token == "pv") {
                    std::string firstMove;
                    if (iss >> firstMove) {
                        pvMove_ = firstMove;
                    }
                }
            }

            if (foundScore) {
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
        explicit EngineInterface(const std::wstring& enginePath) {
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

            if (!CreateProcessW(nullptr, cmd, nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
                return;
            }

            hProcess_ = pi.hProcess;
            hThread_ = pi.hThread;

            CloseHandle(hStdoutWrite);
            CloseHandle(hStdinRead);

            isRunning_ = true;
            listenerThread_ = std::thread(&EngineInterface::listener_loop, this);

            send_command("uci");
            send_command("isready");
        }

        ~EngineInterface() {
            isRunning_ = false;
            if (hStdinWrite_) {
                send_command("quit");
                CloseHandle(hStdinWrite_);
                hStdinWrite_ = nullptr;
            }

            if (hProcess_ && WaitForSingleObject(hProcess_, 4000) == WAIT_TIMEOUT) {
                TerminateProcess(hProcess_, 1);
            }

            if (listenerThread_.joinable()) {
                if (hStdoutRead_) {
                    CloseHandle(hStdoutRead_);
                    hStdoutRead_ = nullptr;
                }
                listenerThread_.join();
            }

            if (hProcess_) CloseHandle(hProcess_);
            if (hThread_)  CloseHandle(hThread_);
        }

        void send_command(const std::string& cmd) {
            if (!isRunning_ || !hStdinWrite_) return;
            std::string full = cmd + "\n";
            DWORD written = 0;
            if (!WriteFile(hStdinWrite_, full.data(), static_cast<DWORD>(full.size()), &written, nullptr)) {
                std::cerr << "Failed to send command: " << cmd << " Error: " << GetLastError() << '\n';
            }
        }
        void start_thinking(const std::string& uci_position_command, int think_time_ms) {
            send_command(uci_position_command);
            go_movetime(think_time_ms);
		}
        void new_game() { 
            std::lock_guard<std::mutex> lock(dataMutex_);
			currentDepth_ = 0;
			currentEval_ = 0;
            bestMove_.clear();
            pvMove_.clear();
            send_command("ucinewgame"); 
        }
        void set_position_startpos(const std::string& uci_string) {
            send_command(uci_string);
        }

        void go_depth(int plies) { send_command("go depth " + std::to_string(plies)); }
        void go_movetime(int ms) { send_command("go movetime " + std::to_string(ms)); }
        void go_infinite() { send_command("go infinite"); }
        void stop() { 
            send_command("stop"); 
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                bestMove_ = "";
            }
        }

        int get_depth(){
            std::lock_guard<std::mutex> lock(dataMutex_);
            return currentDepth_;
        }

        double get_nps() {
            std::lock_guard<std::mutex> lock(dataMutex_);
            return (double)nps_/1'000'000.0;
        }

        EngineInfo get_engine_info() {
            std::lock_guard<std::mutex> lock(dataMutex_);
            return { currentDepth_, (double)nps_ / 1'000'000.0 };
        }
        int get_eval(){
            std::lock_guard<std::mutex> lock(dataMutex_);
            return currentEval_;
        }

        bool is_mate_score(){
            std::lock_guard<std::mutex> lock(dataMutex_);
            return currentEval_ >= 30000 || currentEval_ <= -30000;
        }

        int get_mate_in_plies(){
            std::lock_guard<std::mutex> lock(dataMutex_);
            if (currentEval_ >= 30000) return currentEval_ - 30000;
            if (currentEval_ <= -30000) return -(std::abs(currentEval_) - 30000);
            return 0;
        }

        std::string consume_best_move() {
            std::lock_guard<std::mutex> lock(dataMutex_);
            std::string m = std::move(bestMove_);
            bestMove_.clear();
            return m;
        }

        std::string peek_best_move(){
            std::lock_guard<std::mutex> lock(dataMutex_);
            return pvMove_;
        }
    };
}