struct Base {
    Base(int);
};

struct Derived : Base {
    Derived(int a, int b): Base(a), x(b), y(0) {}

    int x;
    int y;
};

struct Reordered {
    Reordered(): y(1), x(0) {}

    int x;
    int y;
};
