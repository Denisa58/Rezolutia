#include <iostream>
#include <fstream>
#include <cstdlib>
#include <windows.h>
#include <chrono>
#include <iomanip>
#include <set>
#include <algorithm>
#include <psapi.h>  // Pentru a utiliza GetProcessMemoryInfo

#define MAX_CLAUZE 10000
#define MAX_LITERALI 100
#define MAX_LUNGIME_CLAUZA 50
#define FRECVENTA_AFISARE 1000  // Afisam status la fiecare 1000 de pasi

using namespace std;
using namespace std::chrono;

int clauze[MAX_CLAUZE][MAX_LUNGIME_CLAUZA];
int lungime[MAX_CLAUZE];
int nrClauze = 0;
long long pasi = 0;

// Functie pentru masurarea memoriei folosite de procesul curent (în MB)
double memorieFolositaGB() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex); // Obține informații despre memoria sistemului
    DWORDLONG memTotal = statex.ullTotalPhys;
    DWORDLONG memUsed = memTotal - statex.ullAvailPhys; // Memoria folosită
    return memUsed / (1024.0 * 1024.0 * 1024.0); // Convertim în GB
}


// Functie care elimina duplicate si sorteaza clauza noua
void normalizeazaClauza(int clauza[], int &lungime) {
    set<int> literalSet;
    for (int i = 0; i < lungime; i++) {
        literalSet.insert(clauza[i]);
    }

    lungime = 0;
    for (int lit : literalSet) {
        clauza[lungime++] = lit;
    }
}

// Functie pentru citirea clauzelor din fisier .cnf
void citireClauze(const char* numeFisier) {
    ifstream fin(numeFisier);
    if (!fin) {
        cout << "Eroare la deschiderea fisierului!\n";
        exit(1);
    }

    char c;
    string linie;
    while (fin >> c) {
        if (c == 'c') {
            getline(fin, linie);
            continue;
        }
        if (c == 'p') {
            fin >> linie;
            int vars, clauses;
            fin >> vars >> clauses;
            continue;
        }

        fin.putback(c);
        int literal, index = 0;
        while (fin >> literal) {
            if (literal == 0) break;
            if (index < MAX_LUNGIME_CLAUZA) {
                clauze[nrClauze][index++] = literal;
            }
        }
        lungime[nrClauze] = index;
        normalizeazaClauza(clauze[nrClauze], lungime[nrClauze]);
        nrClauze++;
        if (nrClauze >= MAX_CLAUZE) {
            cout << "Eroare: S-a depasit limita maxima de clauze!" << endl;
            exit(1);
        }
    }

    cout << "Clauze citite: " << nrClauze << endl;
}

// Functie care verifica daca doua clauze sunt egale
bool clauzeEgale(int c1[], int l1, int c2[], int l2) {
    if (l1 != l2) return false;
    for (int i = 0; i < l1; i++) {
        if (c1[i] != c2[i]) {
            return false;
        }
    }
    return true;
}

// Functie care verifica daca clauza noua exista deja
bool existaClauza(int noua[], int lenNoua) {
    for (int i = 0; i < nrClauze; i++) {
        if (clauzeEgale(noua, lenNoua, clauze[i], lungime[i])) {
            return true;
        }
    }
    return false;
}

// Functie pentru rezolutie
const char* rezolutie(steady_clock::time_point start) {
    bool modificat = true;
    while (modificat) {
        modificat = false;
        for (int i = 0; i < nrClauze; i++) {
            for (int j = i + 1; j < nrClauze; j++) {
                int countComplement = 0;
                int literalComplement = 0;

                for (int k = 0; k < lungime[i]; k++) {
                    int litA = clauze[i][k];
                    for (int l = 0; l < lungime[j]; l++) {
                        int litB = clauze[j][l];
                        if (litA == -litB) {
                               countComplement++;
                               literalComplement = litA;
                               if (countComplement > 1) break;
                                }

                    }
                }
                if (countComplement != 1) continue;

                    int noua[MAX_LITERALI];
                    int lenNoua = 0;

                    for (int k = 0; k < lungime[i]; k++) {
                        if (clauze[i][k] != literalComplement) {
                            noua[lenNoua++] = clauze[i][k];
                        }
                    }
                    for (int l = 0; l < lungime[j]; l++) {
                        if (clauze[j][l] != -literalComplement) {
                            noua[lenNoua++] = clauze[j][l];
                        }
                    }

                    // Normalizeaza clauza noua
                    normalizeazaClauza(noua, lenNoua);

                    pasi++;

                    // Afisam status la fiecare FRECVENTA_AFISARE pasi
                    if (pasi % FRECVENTA_AFISARE == 0) {
                        auto now = steady_clock::now();
                        auto elapsed_sec = duration_cast<seconds>(now - start).count();
                        double memMB = memorieFolositaGB(); // Schimbare aici

                        cout << "\r" << string(100, ' ') << "\r"; // sterge linia
                        cout << "[Pas " << pasi
                             << "] Timp scurs: " << elapsed_sec << " sec | Memorie folosita: "
                             << fixed << setprecision(2) << memMB << " MB" << flush;
                    }

                    // Verifică dacă clauza generată este goală
                    if (lenNoua == 0) {
                        cout << "\nClauza goala generata! Formula este Nesatisfiabila." << endl;
                        return "Nesatisfiabil";
                    }

                    // Dacă nu există deja clauza în setul de clauze
                    if (!existaClauza(noua, lenNoua)) {
                        for (int m = 0; m < lenNoua; m++) {
                            clauze[nrClauze][m] = noua[m];
                        }
                        lungime[nrClauze] = lenNoua;
                        nrClauze++;
                        modificat = true;
                    }
                }
            }
        }

    return "Satisfiabil";
}

int main() {
    const char* numeFisier = "formula.cnf";
    citireClauze(numeFisier);

    auto start = steady_clock::now();
    const char* rezultat = rezolutie(start);
    auto end = steady_clock::now();

    auto total_time = duration_cast<seconds>(end - start).count();
    double memMB = memorieFolositaGB(); // Schimbare aici

    cout << endl << "Rezultatul: " << rezultat << endl;
    cout << "Timp total de executie: " << total_time << " sec" << endl;
    cout << "Memorie folosita la final: " << fixed << setprecision(2) << memMB << " MB" << endl;

    return 0;
}
