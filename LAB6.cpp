#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sstream>
#include <cstdlib>
#include <sys/wait.h> // Pentru wait

using namespace std;

// Funcție pentru verificarea dacă un număr este prim
bool isPrime(int num) {
    if (num < 2) return false;
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

// Funcția procesului worker
void findPrimes(int start, int end, int writePipe) {
    for (int i = start; i <= end; ++i) {
        if (isPrime(i)) {
            write(writePipe, &i, sizeof(i)); // Scriem numărul prim în pipe
        }
    }
    int endSignal = -1; // Semnal pentru a indica sfârșitul
    write(writePipe, &endSignal, sizeof(endSignal));
    close(writePipe);
}

int main(int argc, char* argv[]) {
    if (argc == 3) {
        // Proces worker
        int start = atoi(argv[1]);
        int end = atoi(argv[2]);
        findPrimes(start, end, STDOUT_FILENO); // Scrie în stdout
        return 0;
    }

    // Proces principal
    const int TOTAL_NUMBERS = 10000;
    const int PROCESS_COUNT = 10;
    const int RANGE_SIZE = TOTAL_NUMBERS / PROCESS_COUNT;

    int pipes[PROCESS_COUNT][2]; // Pipe-uri pentru fiecare proces
    pid_t pids[PROCESS_COUNT];   // ID-urile proceselor

    for (int i = 0; i < PROCESS_COUNT; ++i) {
        if (pipe(pipes[i]) == -1) {
            cerr << "Eroare la crearea pipe-ului!" << endl;
            return 1;
        }

        int start = i * RANGE_SIZE + 1;
        int end = start + RANGE_SIZE - 1;

        if ((pids[i] = fork()) == 0) {
            // Proces worker
            close(pipes[i][0]); // Închidem capătul de citire
            dup2(pipes[i][1], STDOUT_FILENO); // Redirecționăm stdout către pipe
            close(pipes[i][1]); // Închidem capătul original de scriere
            stringstream command;
            command << start << " " << end;
            execlp(argv[0], argv[0], to_string(start).c_str(), to_string(end).c_str(), NULL);
            perror("Eroare la exec");
            exit(1);
        } else if (pids[i] > 0) {
            // Proces părinte
            close(pipes[i][1]); // Închidem capătul de scriere
        } else {
            cerr << "Eroare la fork!" << endl;
            return 1;
        }
    }

    // Citim datele din pipe-uri
    for (int i = 0; i < PROCESS_COUNT; ++i) {
        int prime;
        cout << "Prime numbers from process " << i + 1 << ": ";
        while (true) {
            ssize_t bytesRead = read(pipes[i][0], &prime, sizeof(prime));
            if (bytesRead > 0) {
                if (prime == -1) break; // Semnal de sfârșit
                cout << prime << " ";
            } else {
                break;
            }
        }
        cout << endl;
        close(pipes[i][0]); // Închidem capătul de citire
        waitpid(pids[i], NULL, 0); // Așteptăm ca procesul să termine
    }

    return 0;
}

