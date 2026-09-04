struct Guard {
    Guard();
    ~Guard();
};

void scoped(bool cond) {
    Guard outer;
    if (cond) {
        Guard inner;
    }
    int x = 0;
    (void) x;
}
