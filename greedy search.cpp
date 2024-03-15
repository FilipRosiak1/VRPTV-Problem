#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <math.h>
#include <chrono>


using namespace std;


class Klient {
public:
    int numer, x, y, zapotrzebowanie, od_kiedy, do_kiedy, czas_obslugi;

    Klient(int NO, int XCOORD, int YCOORD, int DEMAND, int READY_TIME, int DUE_TIME, int SERVICE_TIME) {
        numer = NO;
        x = XCOORD;
        y = YCOORD;
        zapotrzebowanie = DEMAND;
        od_kiedy = READY_TIME;
        do_kiedy = DUE_TIME;
        czas_obslugi = SERVICE_TIME;
    }

    int daj_numer() {
        return numer;
    }
    int daj_x() {
        return x;
    }
    int daj_y() {
        return y;
    }
    int daj_zapotrzebowanie() {
        return zapotrzebowanie;
    }
    int od_kiedy_otwarty() {
        return od_kiedy;
    }
    int do_kiedy_otwarty() {
        return do_kiedy;
    }
    int daj_czas_obslugi() {
        return czas_obslugi;
    }

    // liczy odległość między 2 klientami(i/lub magazynem)
    double licz_odleglosc(Klient do_kogo) {
        int dx = do_kogo.daj_x() - this->x;
        int dy = do_kogo.daj_y() - this->y;
        return sqrt(pow(dx, 2) + pow(dy, 2));
    }
private:
};



vector<Klient> klienci; // globalny wektor do którego będą zapisani magazyn i wszyscy klienci(magazyn może być zapisany jako klient 0)


// wczytywanie danych z pliku wejściowego
int wczytanie_pliku(string nazwa, int* tab, int ile_linii) {
    ifstream plik_wejsciowy;
    string linia;
    int k, x, y, zap, otw, zam, ser;
    plik_wejsciowy.open(nazwa);
    if (!plik_wejsciowy.good()) {
        return -1;
    }

    for (int i = 0; i < 4; i++) {
        getline(plik_wejsciowy, linia); // 5 linia to interesujące nas dane, poprzednie pomijamy
        if (linia.size() < 4) i--;
    }
    stringstream bufor1(linia);
    bufor1 >> tab[0] >> tab[1];

    for (int i = 0; i < 2; i++) {
        getline(plik_wejsciowy, linia); // pomijamy 4 zbędne linie
        if (linia.size() < 4) i--;
    }

    // gdy nie czytamy wszystkich, dodaje jedną do odczytania magazynu("ile_linii" klientów + 1 magazyn)
    if (ile_linii != -1) ile_linii++;

    while (getline(plik_wejsciowy, linia) && ile_linii != 0) {
        stringstream bufor2(linia);
        if (bufor2.str().size() < 10) continue;
        bufor2 >> k >> x >> y >> zap >> otw >> zam >> ser;
        klienci.push_back(Klient(k, x, y, zap, otw, zam, ser));
        ile_linii--;
    }
    plik_wejsciowy.close();
    return 0;
}


// sprawdza czy da się obsłużyć konkretnego klienta, pod względem towaru w ciężarówce, czasu dojazdu do klienta i powrotu do magazynu
bool da_sie_obsluzyc(Klient obecny, Klient cel, double ile_minelo, int ile_zostalo) {
    double czas = ile_minelo + obecny.licz_odleglosc(cel);
    
    bool czy_wystarczy = (ile_zostalo >= cel.daj_zapotrzebowanie());    // sprawdza, czy towaru w ciężarówce starczy na konkretnego klienta
    bool czy_dojedzie = (cel.do_kiedy_otwarty() >= czas);   // sprawdza czy zdąży dojechać do klienta(lub magazynu) przed jego zamknięciem
    
    if (czas < cel.od_kiedy_otwarty()) {    // gdy dojedzie przed otwarciem klienta "czeka" do momentu otwarcia
        czas = cel.od_kiedy_otwarty();
    }
    czas = czas + cel.daj_czas_obslugi();
    bool zdazy_wrocic = (klienci[0].do_kiedy_otwarty() >= czas + cel.licz_odleglosc(klienci[0]));   // sprawdza czy zdąży wrócić do magazynu po obsłużeniu
    
    return (czy_wystarczy&&czy_dojedzie&&zdazy_wrocic);
}


double najlepszy_lokalnie_klient(Klient obecny, Klient cel) {
    return obecny.licz_odleglosc(cel);
}


bool czy_dopuszczalne() { // sprawdza, czy da się obsłużyć każdego klienta niezależnie
    for (Klient x : klienci) {
        if (x.daj_numer() == 0) continue; // klienci[0] to magazyn - pomijamy

        if (x.licz_odleglosc(klienci[0]) >= x.od_kiedy_otwarty()) {
            // gdy ciężarówka dojedzie po otwarciu, sprawdza czy zdążyła przed zamknięciem i czy zdąży wrócić do magazynu
            if ((x.licz_odleglosc(klienci[0]) > x.do_kiedy_otwarty()) || (x.licz_odleglosc(klienci[0]) * 2 + x.daj_czas_obslugi() > klienci[0].do_kiedy_otwarty())) {
                return false;
            }
        }
        else {
            // gdy dojedzie przed czasem, sprawdza czy zdąży wrócić do magazynu po odsłużeniu klienta
            if (x.licz_odleglosc(klienci[0]) + x.daj_czas_obslugi() + x.od_kiedy_otwarty() > klienci[0].do_kiedy_otwarty()) {
                return false;
            }
        }
    }
    return true;
}


void zachlanny(ofstream& plik_wyjsciowy, int cap) {
    if (!czy_dopuszczalne()) {
        plik_wyjsciowy << -1;
        cout << -1 << endl;
        return;
    }

    int przejechane_trasy = 0;
    int przejechane_trasy2 = 0;
    double sumaryczny_czas = 0.0;
    double czas = 0.0;
    double ladownosc = cap;
    Klient obecny_klient = klienci[0];
    vector<vector<int>> trasy;
    vector<int> trasa;
    trasy.push_back(trasa);


    while (true) {
        if (klienci.size() == 1) {  //kiedy w tablicy zostanie sam magazyn (klienci[0])
            for (int i = 0; i < przejechane_trasy; i++) {
                if (!trasy[i].empty()) {
                    przejechane_trasy2++;
                }
            }
            cout.precision(6);
            cout << przejechane_trasy2 << " " << fixed << sumaryczny_czas << endl;
            plik_wyjsciowy.precision(6);
            plik_wyjsciowy << przejechane_trasy2 << " " << fixed << sumaryczny_czas << endl;
            for (int i = 0; i < przejechane_trasy2; i++) {
                for (int tr : trasy[i]) {
                    cout << tr << " ";
                    plik_wyjsciowy << tr << " ";
                }
                cout << endl;
                plik_wyjsciowy << endl;
            }
            break;
        }

        int nastepny_klient = 1; // pomijamy magazyn (klienci[0] = magazyn), przypisujemy pierwszego klienta z listy
        for (int i = 2; i < (int)klienci.size(); i++) {  // szukanie najlepszego lokalnego następnego klienta
            if (da_sie_obsluzyc(obecny_klient, klienci[i], czas, ladownosc) &&
                najlepszy_lokalnie_klient(obecny_klient, klienci[i]) < najlepszy_lokalnie_klient(obecny_klient, klienci[nastepny_klient])) {
                nastepny_klient = i;
            }
        }

        // gdy nie znajdzie nasteępnego pasującego klienta i piewszy z listy(ustawiany domyślnie) nie może być obsłużony,
        // przypisuje wartość -1, która reprezentuje brak następnego klienta
        if (nastepny_klient == 1 && !da_sie_obsluzyc(obecny_klient, klienci[1], czas, ladownosc)) {
            nastepny_klient = -1;
        }

        if (nastepny_klient != -1) {    // jeśli znalazło następnego klienta...
            trasy[przejechane_trasy].push_back(klienci[nastepny_klient].daj_numer()); // dodajemy jego id do obecnej trasy

            sumaryczny_czas = sumaryczny_czas + obecny_klient.licz_odleglosc(klienci[nastepny_klient]);
            czas = czas + obecny_klient.licz_odleglosc(klienci[nastepny_klient]);

            if (klienci[nastepny_klient].od_kiedy_otwarty() > czas) {   // gdy dojedzie przed czasem otwarcia klienta, "czeka" i dodaje czas oczekiwania
                sumaryczny_czas = sumaryczny_czas + (klienci[nastepny_klient].od_kiedy_otwarty() - czas);
                czas = klienci[nastepny_klient].od_kiedy_otwarty(); 
            }

            ladownosc = ladownosc - klienci[nastepny_klient].daj_zapotrzebowanie();

            sumaryczny_czas = sumaryczny_czas + klienci[nastepny_klient].daj_czas_obslugi(); 
            czas = czas + klienci[nastepny_klient].daj_czas_obslugi();

            obecny_klient = klienci[nastepny_klient];   // następny klient staje sie obecnym klientem
            klienci.erase(klienci.begin() + nastepny_klient);   // usunięcie obsłużonego (aktualnie obecnego) klienta
        }

        if (ladownosc == 0 || nastepny_klient == -1) { // jeśli nie ma następnego klienta lub skończył się ładunek następuje podsumowanie trasy 
            przejechane_trasy++;    
            sumaryczny_czas = sumaryczny_czas + obecny_klient.licz_odleglosc(klienci[0]);
            czas = 0.0; // resetujemy czas trasy
            ladownosc = cap; // reset ładowności
            obecny_klient = klienci[0]; // ciężarówka zaczyna z magazynu
            vector<int> trasa;
            trasy.push_back(trasa);
        }

        if (klienci.size() == 1) {  // gdy skończyli się wszyscy klienci(finalizujemy ostatnią trasę)
            przejechane_trasy++;   
            sumaryczny_czas += obecny_klient.licz_odleglosc(klienci[0]);
        }
    }

}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("polecenie:  CVRPTW_zachlanne.exe <plik wejsciowy> <plik wyjsciowy> <ilu klientow wczytac (-1 - wszyscy)>\n");
        return -1;
    }

    auto start = chrono::high_resolution_clock::now();

    ofstream plik_wyjsciowy;
    plik_wyjsciowy.open(argv[2], ofstream::out);
    int pojazdy[2];    // 0 - liczba_pojazdow, 1 - ladownosc;

    if (wczytanie_pliku(argv[1], pojazdy, stoi(argv[3])) == -1) {
        cout << "Nie udalo sie otworzyc pliku :c" << endl;
        return -1;
    }

    zachlanny(plik_wyjsciowy, pojazdy[1]);
    plik_wyjsciowy.close();

    auto end = chrono::high_resolution_clock::now();
    double czas_wykonywania_programu = chrono::duration_cast<chrono::nanoseconds>(end - start).count();
    czas_wykonywania_programu *= 1e-9;
    cout << "Czas wykonywania programu: " << czas_wykonywania_programu << setprecision(9) << "s." << endl;

    return 0;
}