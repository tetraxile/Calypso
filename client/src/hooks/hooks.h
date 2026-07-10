namespace cly::hooks {
struct Hooks {
	static void setup() {
		setupInit();
		setupTas();
		setupFs();
		setupServer();
		setupBasic();
	}

private:
	static void setupInit();
	static void setupBasic();
	static void setupServer();
	static void setupFs();
	static void setupTas();
};
} // namespace cly::hooks
