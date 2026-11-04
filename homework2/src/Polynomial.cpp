// Homework 2 - Polynomial
// Design focus: the required dynamic Term array, safe object lifetime,
// normalized sparse polynomials, reliable I/O, and reproducible tests.
// Build: g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic Polynomial.cpp -o Polynomial

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

class Polynomial;  // 前向宣告：與題目要求的類別關係保持一致。

class Term {
    friend class Polynomial;
private:
    float coef;  // 係數
    int exp;     // 指數，規定為非負整數
};

namespace detail {

// 文字輸入採嚴格檢查，避免 2x、NaN、INF 與多餘欄位被悄悄接受。
int parseNonnegativeInt(const string& text, const string& field) {
    if (text.empty()) throw invalid_argument(field + "不可為空");
    const char* begin = text.data();
    const char* end = begin + text.size();
    if (*begin == '+') ++begin;
    if (begin == end) throw invalid_argument(field + "必須是非負整數");
    int result = 0;
    auto parsed = from_chars(begin, end, result);
    if (parsed.ec != errc{} || parsed.ptr != end || result < 0)
        throw invalid_argument(field + "必須是 int 範圍內的非負整數");
    return result;
}

float parseFiniteFloat(const string& text, const string& field) {
    if (text.empty()) throw invalid_argument(field + "不可為空");
    errno = 0;
    char* end = nullptr;
    const float result = strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || errno == ERANGE || !isfinite(result))
        throw invalid_argument(field + "必須是可表示的有限數字");
    return result;
}

// 使用 C++17 shortest round-trip 格式，0.1 不會顯示為 0.100000001。
string formatFloat(float value) {
    char buffer[80];
    auto written = to_chars(buffer, buffer + sizeof(buffer), value, chars_format::general);
    if (written.ec != errc{}) throw runtime_error("數字格式化失敗");
    return string(buffer, written.ptr);
}

float checkedFloat(long double value) {
    if (!isfinite(value) || fabsl(value) > static_cast<long double>(numeric_limits<float>::max()))
        throw overflow_error("係數結果超出 float 可表示範圍");
    return static_cast<float>(value);
}

string readNonemptyLine(istream& input) {
    string line;
    if (!getline(input >> ws, line)) throw runtime_error("輸入已結束");
    return line;
}

}  // namespace detail

class Polynomial {
    friend istream& operator>>(istream& input, Polynomial& poly);
    friend ostream& operator<<(ostream& output, const Polynomial& poly);
private:
    Term* termArray;  // 主儲存：題目指定的動態 Term 陣列
    int capacity;     // 已分配容量
    int terms;        // 非零項數量；指數由大至小，且不重複

    void reserve(int required) {
        if (required <= capacity) return;
        if (required < 0) throw length_error("陣列容量無效");
        int nextCapacity = capacity ? capacity : 1;
        while (nextCapacity < required) {
            nextCapacity = nextCapacity > INT_MAX / 2 ? required : nextCapacity * 2;
        }
        unique_ptr<Term[]> replacement(new Term[nextCapacity]);
        for (int i = 0; i < terms; ++i) replacement[i] = termArray[i];
        delete[] termArray;
        termArray = replacement.release();
        capacity = nextCapacity;
    }

    // 僅用在資料已按指數遞減排序、沒有同次項的情況。
    void appendSorted(long double coefficient, int exponent) {
        float value = detail::checkedFloat(coefficient);
        if (value == 0.0f) return;
        reserve(terms + 1);
        termArray[terms].coef = value;
        termArray[terms].exp = exponent;
        ++terms;
    }

public:
    // Rule of Five：動態陣列必須自行處理複製、移動和解構。
    Polynomial() : termArray(nullptr), capacity(0), terms(0) {}

    Polynomial(const Polynomial& other)
        : termArray(nullptr), capacity(0), terms(0) {
        reserve(other.terms);
        for (int i = 0; i < other.terms; ++i) termArray[i] = other.termArray[i];
        terms = other.terms;
    }

    Polynomial(Polynomial&& other) noexcept
        : termArray(other.termArray), capacity(other.capacity), terms(other.terms) {
        other.termArray = nullptr;
        other.capacity = 0;
        other.terms = 0;
    }

    // copy-and-swap：先複製成功，再換掉舊資料；自我賦值也安全。
    Polynomial& operator=(Polynomial other) noexcept {
        swap(other);
        return *this;
    }

    ~Polynomial() { delete[] termArray; }

    void swap(Polynomial& other) noexcept {
        std::swap(termArray, other.termArray);
        std::swap(capacity, other.capacity);
        std::swap(terms, other.terms);
    }

    // 題目要求的 Add：雙指標掃描，O(m+n)。
    Polynomial Add(Polynomial poly) {
        Polynomial result;
        result.reserve(terms + poly.terms);
        int left = 0, right = 0;
        while (left < terms && right < poly.terms) {
            if (termArray[left].exp > poly.termArray[right].exp) {
                result.appendSorted(termArray[left].coef, termArray[left].exp);
                ++left;
            } else if (termArray[left].exp < poly.termArray[right].exp) {
                result.appendSorted(poly.termArray[right].coef, poly.termArray[right].exp);
                ++right;
            } else {
                const long double sum = static_cast<long double>(termArray[left].coef)
                                      + static_cast<long double>(poly.termArray[right].coef);
                result.appendSorted(sum, termArray[left].exp);
                ++left;
                ++right;
            }
        }
        while (left < terms) {
            result.appendSorted(termArray[left].coef, termArray[left].exp);
            ++left;
        }
        while (right < poly.terms) {
            result.appendSorted(poly.termArray[right].coef, poly.termArray[right].exp);
            ++right;
        }
        return result;
    }

    // 題目要求的 Mult：先產生所有乘積，按指數排序，再合併同次項。
    Polynomial Mult(Polynomial poly) {
        Polynomial result;
        if (terms == 0 || poly.terms == 0) return result;

        struct ProductTerm {
            int exp;
            long double coef;
        };

        const size_t m = static_cast<size_t>(terms);
        const size_t n = static_cast<size_t>(poly.terms);
        if (m > numeric_limits<size_t>::max() / n)
            throw length_error("乘法項數過大");
        const size_t count = m * n;
        if (count > numeric_limits<size_t>::max() / sizeof(ProductTerm))
            throw length_error("乘法暫存空間過大");
        unique_ptr<ProductTerm[]> products(new ProductTerm[count]);
        size_t written = 0;

        for (int i = 0; i < terms; ++i) {
            for (int j = 0; j < poly.terms; ++j) {
                const long long expSum = static_cast<long long>(termArray[i].exp)
                                       + static_cast<long long>(poly.termArray[j].exp);
                if (expSum > INT_MAX) throw overflow_error("相乘後的指數超出 int 範圍");
                products[written++] = {
                    static_cast<int>(expSum),
                    static_cast<long double>(termArray[i].coef) * poly.termArray[j].coef
                };
            }
        }

        sort(products.get(), products.get() + count,
             [](const ProductTerm& a, const ProductTerm& b) { return a.exp > b.exp; });

        for (size_t start = 0; start < count;) {
            const int exponent = products[start].exp;
            long double coefficient = 0.0L;
            do {
                coefficient += products[start].coef;
                ++start;
            } while (start < count && products[start].exp == exponent);
            result.appendSorted(coefficient, exponent);
        }
        return result;
    }

    // 題目要求的 Eval：稀疏版 Horner 法，以 long double 降低中間誤差。
    float Eval(float f) {
        if (!isfinite(f)) throw invalid_argument("代入值必須是有限數字");
        if (terms == 0) return 0.0f;

        long double sum = termArray[0].coef;
        int previousExponent = termArray[0].exp;
        for (int i = 1; i < terms; ++i) {
            const int gap = previousExponent - termArray[i].exp;
            const long double power = pow(static_cast<long double>(f), gap);
            sum = sum * power + termArray[i].coef;
            if (!isfinite(sum)) throw overflow_error("代入計算的中間結果過大");
            previousExponent = termArray[i].exp;
        }
        sum *= pow(static_cast<long double>(f), previousExponent);
        return detail::checkedFloat(sum);
    }

    int TermCount() const noexcept { return terms; }
    int Capacity() const noexcept { return capacity; }
    int Degree() const noexcept { return terms == 0 ? -1 : termArray[0].exp; }
    size_t ReservedBytes() const noexcept { return static_cast<size_t>(capacity) * sizeof(Term); }

    // 一行內：項數 係數1 指數1 係數2 指數2 ...，順序可以任意且可重複。
    static Polynomial Parse(const string& line) {
        istringstream source(line);
        string token;
        if (!(source >> token)) throw invalid_argument("缺少項數");
        const int count = detail::parseNonnegativeInt(token, "項數");
        // 限制單行項數，避免錯誤輸入不合理地耗盡記憶體。
        constexpr int MAX_INPUT_TERMS = 200000;
        if (count > MAX_INPUT_TERMS)
            throw length_error("單次輸入項數不可超過 200000");

        unique_ptr<Term[]> entries(new Term[count]);
        for (int i = 0; i < count; ++i) {
            string coefText, expText;
            if (!(source >> coefText >> expText))
                throw invalid_argument("資料數量不足：每項需提供係數與指數");
            entries[i].coef = detail::parseFiniteFloat(coefText, "係數");
            entries[i].exp = detail::parseNonnegativeInt(expText, "指數");
        }
        if (source >> token) throw invalid_argument("存在多餘欄位");
        sort(entries.get(), entries.get() + count,
             [](const Term& a, const Term& b) { return a.exp > b.exp; });

        Polynomial result;
        result.reserve(count);
        for (int i = 0; i < count;) {
            const int exponent = entries[i].exp;
            long double total = 0;
            do {
                total += entries[i].coef;
                ++i;
            } while (i < count && entries[i].exp == exponent);
            result.appendSorted(total, exponent);
        }
        return result;
    }

    string ToString() const {
        if (terms == 0) return "0";
        ostringstream output;
        for (int i = 0; i < terms; ++i) {
            const float coefficient = termArray[i].coef;
            const int exponent = termArray[i].exp;
            if (i == 0) {
                if (coefficient < 0) output << "-";
            } else {
                output << (coefficient < 0 ? " - " : " + ");
            }
            const float magnitude = fabs(coefficient);
            if (exponent == 0 || magnitude != 1.0f)
                output << detail::formatFloat(magnitude);
            if (exponent > 0) {
                output << 'x';
                if (exponent > 1) output << '^' << exponent;
            }
        }
        return output.str();
    }

    // 輸出可再次 Parse 的機器格式，適合存檔；不同於好讀的 << 輸出。
    void Save(ostream& output) const {
        output << terms;
        for (int i = 0; i < terms; ++i) {
            output << ' ' << detail::formatFloat(termArray[i].coef)
                   << ' ' << termArray[i].exp;
        }
        output << '\n';
    }
};

istream& operator>>(istream& input, Polynomial& poly) {
    string line;
    if (!getline(input >> ws, line)) return input;
    try {
        Polynomial candidate = Polynomial::Parse(line);
        poly.swap(candidate);  // 全部檢查成功才替換，不破壞原資料。
    } catch (const exception&) {
        input.setstate(ios::failbit);
    }
    return input;
}

ostream& operator<<(ostream& output, const Polynomial& poly) {
    return output << poly.ToString();
}

namespace app {

Polynomial readPolynomialOrThrow(const string& prompt) {
    while (true) {
        cout << prompt << flush;
        const string line = detail::readNonemptyLine(cin);
        try {
            return Polynomial::Parse(line);
        } catch (const exception& error) {
            cout << "輸入錯誤：" << error.what() << "，請重新輸入。\n";
        }
    }
}

float readValueOrThrow() {
    while (true) {
        cout << "請輸入 x：" << flush;
        const string line = detail::readNonemptyLine(cin);
        istringstream parser(line);
        string token, extra;
        if ((parser >> token) && !(parser >> extra)) {
            try {
                return detail::parseFiniteFloat(token, "x");
            } catch (const exception& error) {
                cout << "輸入錯誤：" << error.what() << "，請重新輸入。\n";
                continue;
            }
        }
        cout << "輸入錯誤：請只輸入一個數字。\n";
    }
}

bool savePair(const string& filename, const Polynomial& a, const Polynomial& b) {
    ofstream file(filename, ios::out | ios::trunc);
    if (!file) return false;
    a.Save(file);
    b.Save(file);
    file.flush();
    return static_cast<bool>(file);
}

void loadPair(const string& filename, Polynomial& a, Polynomial& b) {
    ifstream file(filename);
    if (!file) throw runtime_error("無法開啟檔案");
    string first, second, rest;
    if (!getline(file, first) || !getline(file, second))
        throw invalid_argument("檔案至少需要兩行多項式資料");
    while (getline(file, rest)) {
        if (rest.find_first_not_of(" \t\r\n") != string::npos)
            throw invalid_argument("檔案包含多餘資料");
    }
    Polynomial nextA = Polynomial::Parse(first);
    Polynomial nextB = Polynomial::Parse(second);
    a.swap(nextA);
    b.swap(nextB);
}

void printStats(const char* name, const Polynomial& p) {
    cout << name << ": " << p << "\n"
         << "  非零項數：" << p.TermCount()
         << "，最高次方：" << (p.Degree() < 0 ? string("無（零多項式）") : to_string(p.Degree()))
         << "，容量：" << p.Capacity()
         << "，Term 陣列配置：約 " << p.ReservedBytes() << " bytes\n";
}

bool runSelfTests(ostream& output) {
    int passed = 0, total = 0;
    auto check = [&](const string& label, bool correct) {
        ++total;
        if (correct) ++passed;
        output << (correct ? "[PASS] " : "[FAIL] ") << label << '\n';
    };
    auto parse = [](const string& s) { return Polynomial::Parse(s); };
    auto nearlyEqual = [](float actual, long double expected) {
        return fabsl(static_cast<long double>(actual) - expected)
             <= 0.0001L * max(1.0L, fabsl(expected));
    };

    Polynomial a = parse("3 3 2 2 1 1 0");
    Polynomial b = parse("2 1 1 -1 0");
    check("基本輸入與輸出", a.ToString() == "3x^2 + 2x + 1" && b.ToString() == "x - 1");
    check("零多項式", parse("0").ToString() == "0");
    check("重複指數與零項消去", parse("4 5 2 -5 2 2 0 -2 0").ToString() == "0");
    check("亂序輸入自動排序", parse("3 1 0 2 1 3 4").ToString() == "3x^4 + 2x + 1");
    check("係數 1 / -1 與符號顯示", parse("3 1 3 -1 1 -1 0").ToString() == "x^3 - x - 1");
    check("加法", a.Add(b).ToString() == "3x^2 + 3x");
    check("乘法", a.Mult(b).ToString() == "3x^3 - x^2 - x - 1");
    check("A(2) = 17", nearlyEqual(a.Eval(2), 17));
    check("負數代入與奇次方", nearlyEqual(parse("1 2 3").Eval(-2), -16));
    check("零多項式加乘", parse("0").Add(a).ToString() == a.ToString()
          && parse("0").Mult(a).ToString() == "0");

    Polynomial copy = a;
    a = parse("1 9 0");
    check("深複製不共用記憶體", copy.ToString() == "3x^2 + 2x + 1");
    copy = copy;
    check("自我賦值", copy.ToString() == "3x^2 + 2x + 1");
    Polynomial moved = std::move(a);
    check("移動建構", moved.ToString() == "9" && a.ToString() == "0");

    Polynomial preserved = parse("1 7 0");
    istringstream badInput("1 8 -1\n");
    badInput >> preserved;
    check("不合法輸入不破壞原物件", !badInput && preserved.ToString() == "7");
    bool extraRejected = false, nanRejected = false, expOverflow = false, coefOverflow = false;
    try { (void)parse("0 1"); } catch (const exception&) { extraRejected = true; }
    try { (void)parse("1 nan 0"); } catch (const exception&) { nanRejected = true; }
    try { (void)parse("1 1 2147483647").Mult(parse("1 1 1")); }
    catch (const overflow_error&) { expOverflow = true; }
    try { (void)parse("1 3e38 0").Mult(parse("1 3 0")); }
    catch (const overflow_error&) { coefOverflow = true; }
    check("拒絕多餘輸入", extraRejected);
    check("拒絕 NaN", nanRejected);
    check("乘法指數溢位防護", expOverflow);
    check("乘法係數溢位防護", coefOverflow);

    ostringstream saved;
    copy.Save(saved);
    Polynomial loaded = parse(saved.str());
    check("儲存與讀取資料可往返", loaded.ToString() == copy.ToString());

    // 固定亂數種子，與獨立的直接代入值交叉檢查 100 組案例。
    mt19937 generator(20261104);
    uniform_int_distribution<int> coefficient(-2, 2);
    uniform_int_distribution<int> exponent(0, 6);
    uniform_int_distribution<int> xValue(-2, 2);
    bool allRandomCorrect = true;
    for (int trial = 0; trial < 100; ++trial) {
        ostringstream textA, textB;
        textA << 5; textB << 5;
        long double expectedA = 0, expectedB = 0;
        const int x = xValue(generator);
        for (int j = 0; j < 5; ++j) {
            int c = coefficient(generator), e = exponent(generator);
            textA << ' ' << c << ' ' << e;
            expectedA += c * pow(static_cast<long double>(x), e);
        }
        for (int j = 0; j < 5; ++j) {
            int c = coefficient(generator), e = exponent(generator);
            textB << ' ' << c << ' ' << e;
            expectedB += c * pow(static_cast<long double>(x), e);
        }
        Polynomial p = parse(textA.str()), q = parse(textB.str());
        if (!nearlyEqual(p.Eval(static_cast<float>(x)), expectedA)
            || !nearlyEqual(q.Eval(static_cast<float>(x)), expectedB)
            || !nearlyEqual(p.Add(q).Eval(static_cast<float>(x)), expectedA + expectedB)
            || !nearlyEqual(p.Mult(q).Eval(static_cast<float>(x)), expectedA * expectedB)) {
            allRandomCorrect = false;
            break;
        }
    }
    check("100 組固定種子隨機交叉驗證", allRandomCorrect);
    output << "總結果：" << passed << '/' << total << " 通過\n";
    return passed == total;
}

// 只量測核心運算，避免把輸入提示和檔案 I/O 混入測量。
void runBenchmark(ostream& output) {
    output << "項數n,加法平均毫秒(200次),乘法平均毫秒(3次),乘積非零項數\n";
    for (int n : {100, 200, 400}) {
        ostringstream source;
        source << n;
        for (int e = n - 1; e >= 0; --e) source << " 1 " << e;
        Polynomial a = Polynomial::Parse(source.str());
        Polynomial b = a;
        size_t checkSum = 0;
        using clock = chrono::steady_clock;
        const auto addStart = clock::now();
        for (int i = 0; i < 200; ++i) checkSum += a.Add(b).TermCount();
        const auto addFinish = clock::now();
        const auto multStart = clock::now();
        for (int i = 0; i < 3; ++i) checkSum += a.Mult(b).TermCount();
        const auto multFinish = clock::now();
        double addMilliseconds = chrono::duration<double, milli>(addFinish - addStart).count() / 200.0;
        double multMilliseconds = chrono::duration<double, milli>(multFinish - multStart).count() / 3.0;
        output << n << ',' << fixed << setprecision(6)
               << addMilliseconds << ',' << multMilliseconds << ',' << (2 * n - 1) << '\n';
        // 防止測試循環變成完全不可觀察的無用計算。
        if (checkSum == 0) throw logic_error("基準測試資料異常");
    }
}

int batchMode() {
    try {
        const Polynomial a = Polynomial::Parse(detail::readNonemptyLine(cin));
        const Polynomial b = Polynomial::Parse(detail::readNonemptyLine(cin));
        istringstream xLine(detail::readNonemptyLine(cin));
        string token, extra;
        if (!(xLine >> token) || (xLine >> extra))
            throw invalid_argument("x 必須是一個數字");
        const float x = detail::parseFiniteFloat(token, "x");
        // 指定核心方法是非 const；使用副本保留題目原簽名。
        Polynomial left = a, right = b;
        const Polynomial sum = left.Add(right);
        const Polynomial product = left.Mult(right);
        const float resultA = left.Eval(x), resultB = right.Eval(x);
        cout << "A = " << a << '\n'
             << "B = " << b << '\n'
             << "A + B = " << sum << '\n'
             << "A * B = " << product << '\n'
             << "A(" << detail::formatFloat(x) << ") = " << detail::formatFloat(resultA) << '\n'
             << "B(" << detail::formatFloat(x) << ") = " << detail::formatFloat(resultB) << '\n';
        return 0;
    } catch (const exception& error) {
        cerr << "輸入或運算錯誤：" << error.what() << '\n';
        return 1;
    }
}

int interactiveMode() {
    cout << "=== Homework 2 | Polynomial 多項式運算系統 ===\n"
         << "輸入格式（每個多項式各一行）：項數 係數1 指數1 係數2 指數2 ...\n"
         << "例如：3 3 2 2 1 1 0，代表 3x^2 + 2x + 1。\n"
         << "允許亂序及同次項；指數限非負整數，係數可輸入有限小數。\n\n";
    Polynomial a, b;
    try {
        a = readPolynomialOrThrow("請輸入 A：");
        b = readPolynomialOrThrow("請輸入 B：");
    } catch (const exception&) {
        cout << "輸入結束。\n";
        return 0;
    }

    while (true) {
        cout << "\n========== 操作選單 ==========\n"
             << "1. 顯示 A、B\n"
             << "2. A + B\n"
             << "3. A * B\n"
             << "4. 計算 A(x)\n"
             << "5. 計算 B(x)\n"
             << "6. 重新輸入 A\n"
             << "7. 重新輸入 B\n"
             << "8. 儲存 A、B 到檔案\n"
             << "9. 從檔案載入 A、B\n"
             << "10. 顯示項數、次數與容量\n"
             << "11. 執行內建正確性測試\n"
             << "12. 執行效能基準測試\n"
             << "0. 結束\n"
             << "請選擇：" << flush;
        string choiceText;
        if (!getline(cin, choiceText)) break;
        istringstream choiceInput(choiceText);
        string token, extra;
        if (!(choiceInput >> token) || (choiceInput >> extra)) {
            cout << "選單輸入無效，請重新選擇。\n";
            continue;
        }
        int choice = -1;
        try { choice = detail::parseNonnegativeInt(token, "選單選項"); }
        catch (const exception&) { cout << "選單輸入無效，請重新選擇。\n"; continue; }
        if (choice == 0) break;

        try {
            switch (choice) {
                case 1:
                    cout << "A = " << a << "\nB = " << b << '\n';
                    break;
                case 2:
                    cout << "A + B = " << a.Add(b) << '\n';
                    break;
                case 3:
                    cout << "A * B = " << a.Mult(b) << '\n';
                    break;
                case 4: {
                    const float x = readValueOrThrow();
                    cout << "A(" << detail::formatFloat(x) << ") = "
                         << detail::formatFloat(a.Eval(x)) << '\n';
                    break;
                }
                case 5: {
                    const float x = readValueOrThrow();
                    cout << "B(" << detail::formatFloat(x) << ") = "
                         << detail::formatFloat(b.Eval(x)) << '\n';
                    break;
                }
                case 6:
                    a = readPolynomialOrThrow("請重新輸入 A：");
                    cout << "A 已更新。\n";
                    break;
                case 7:
                    b = readPolynomialOrThrow("請重新輸入 B：");
                    cout << "B 已更新。\n";
                    break;
                case 8: {
                    cout << "儲存檔案名稱：" << flush;
                    const string filename = detail::readNonemptyLine(cin);
                    cout << (savePair(filename, a, b) ? "儲存成功。\n" : "儲存失敗。\n");
                    break;
                }
                case 9: {
                    cout << "讀取檔案名稱：" << flush;
                    const string filename = detail::readNonemptyLine(cin);
                    loadPair(filename, a, b);
                    cout << "讀取成功。\n";
                    break;
                }
                case 10:
                    printStats("A", a);
                    printStats("B", b);
                    break;
                case 11:
                    runSelfTests(cout);
                    break;
                case 12:
                    runBenchmark(cout);
                    break;
                default:
                    cout << "沒有這個選項，請重新選擇。\n";
                    break;
            }
        } catch (const exception& error) {
            if (!cin) { cout << "輸入結束。\n"; break; }
            cout << "操作失敗：" << error.what() << '\n';
        }
    }
    cout << "程式結束。\n";
    return 0;
}

}  // namespace app

int main(int argc, char* argv[]) {
    if (argc == 1) return app::interactiveMode();
    if (argc == 2) {
        const string mode = argv[1];
        if (mode == "--batch") return app::batchMode();
        if (mode == "--self-test") return app::runSelfTests(cout) ? 0 : 1;
        if (mode == "--benchmark") { app::runBenchmark(cout); return 0; }
    }
    cerr << "使用方式：Polynomial [--batch | --self-test | --benchmark]\n";
    return 1;
}
