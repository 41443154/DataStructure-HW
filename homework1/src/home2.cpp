#include "header.h"

using namespace std;

void printSubset(string current[], int size, bool& first)
{
    if (!first)
    {
        cout << ", ";
    }

    first = false;

    cout << "(";

    for (int i = 0; i < size; i++)
    {
        cout << current[i];

        if (i != size - 1)
        {
            cout << ",";
        }
    }

    cout << ")";
}

void generate(
    string S[],
    int n,
    int start,
    int need,
    string current[],
    int currentSize,
    bool& first)
{
    if (need == 0)
    {
        printSubset(current, currentSize, first);
        return;
    }

    for (int i = start; i <= n - need; i++)
    {
        current[currentSize] = S[i];

        generate(
            S,
            n,
            i + 1,
            need - 1,
            current,
            currentSize + 1,
            first
        );
    }
}

void powerset(string S[], int n)
{
    string* current = new string[n];

    bool first = true;

    cout << "{";

    for (int size = 0; size <= n; size++)
    {
        generate(
            S,
            n,
            0,
            size,
            current,
            0,
            first
        );
    }

    cout << "}" << endl;

    delete[] current;
}

int main()
{
    int n;

    cout << "Input set size: ";
    cin >> n;

    if (n < 0)
    {
        cout << "Invalid size" << endl;
        return 0;
    }

    string* S = new string[n];

    cout << "Input elements: ";

    for (int i = 0; i < n; i++)
    {
        cin >> S[i];
    }

    powerset(S, n);

    delete[] S;

    return 0;
}