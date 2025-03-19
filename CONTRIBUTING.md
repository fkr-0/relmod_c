# Contributing to X11 Menu Extension System

Thank you for considering contributing to the menu extension system! This document provides guidelines and workflows for contributing effectively.

## Development Environment

### Required Tools
- C compiler (gcc/clang)
- Make build system
- X11/XCB development libraries
- Cairo development libraries
- Python 3.x (for test reporting)
- Valgrind (for memory testing)

### Setup
```bash
# Install dependencies on Ubuntu/Debian
sudo apt-get install \
    build-essential \
    libxcb1-dev \
    libxcb-ewmh-dev \
    libcairo2-dev \
    libx11-dev \
    python3 \
    valgrind

# Clone repository
git clone https://github.com/username/menu-extension.git
cd menu-extension

# Build project
make

# Run tests
make tests
```

## Code Style

### C Code Guidelines

1. **Naming Conventions**
   ```c
   // Functions: snake_case
   void menu_manager_create();
   void menu_handle_input();

   // Types: PascalCase
   typedef struct MenuConfig {
       // ...
   } MenuConfig;

   // Variables: snake_case
   MenuConfig* menu_config;
   int item_count;

   // Constants: UPPER_SNAKE_CASE
   #define MAX_MENU_ITEMS 100
   ```

2. **Function Organization**
   ```c
   // Public API
   MenuManager* menu_manager_create();

   // Internal helpers (static)
   static void cleanup_resources();

   // Callbacks
   static void handle_input_event();
   ```

3. **Comments and Documentation**
   ```c
   /**
    * Creates a new menu with the given configuration.
    *
    * @param config Menu configuration structure
    * @return New menu instance or NULL on error
    */
   Menu* menu_create(MenuConfig* config);
   ```

4. **Error Handling**
   ```c
   Menu* menu = menu_create(config);
   if (!menu) {
       fprintf(stderr, "Failed to create menu\n");
       return NULL;
   }
   ```

### Commit Guidelines

1. **Commit Message Format**
   ```
   [component] Brief description

   Detailed explanation of changes, including:
   - Why the change was made
   - Any important implementation details
   - Related issue numbers
   ```

2. **Example Commits**
   ```
   [menu] Add support for nested menus

   - Implement menu item nesting structure
   - Add parent/child relationship tracking
   - Update rendering to handle submenus
   - Add navigation for nested items

   Fixes #123
   ```

## Development Workflow

1. **Creating Features**
   ```bash
   # Create feature branch
   git checkout -b feature/new-feature

   # Make changes and run tests
   make clean && make
   make tests

   # Commit changes
   git commit -m "[component] Add new feature"

   # Push branch
   git push origin feature/new-feature
   ```

2. **Testing**
   - Run unit tests: `make test`
   - Run with sanitizers: `make test-sanitize`
   - Check memory: `make test-memory`
   - Generate coverage: `make test-coverage`

3. **Code Review Process**
   1. Create pull request
   2. Wait for CI checks
   3. Address review feedback
   4. Update documentation
   5. Maintain clean commit history

## Adding New Features

1. **Component Updates**
   - Update relevant header files
   - Implement new functionality
   - Add unit tests
   - Update documentation

2. **Example: Adding Menu Type**
   ```c
   // In menu.h
   typedef struct {
       // New menu type configuration
   } NewMenuConfig;

   // In new_menu.c
   Menu* new_menu_create(NewMenuConfig* config) {
       // Implementation
   }

   // In tests/test_new_menu.c
   static void test_new_menu() {
       // Test implementation
   }
   ```

## Testing Guidelines

1. **Unit Tests**
   - One test file per component
   - Clear test names
   - Comprehensive coverage
   - Clean setup/teardown

2. **Integration Tests**
   - Test component interactions
   - Cover common workflows
   - Test error conditions

3. **Performance Tests**
   - Benchmark critical paths
   - Test with varying loads
   - Compare against baselines

4. **Memory Tests**
   - Check for leaks
   - Monitor usage patterns
   - Test resource cleanup

## Documentation

1. **Code Documentation**
   - Header file API docs
   - Implementation notes
   - Usage examples

2. **Project Documentation**
   - Update README.md
   - Add to TUTORIAL.md
   - Update ARCHITECTURE.md

3. **Test Documentation**
   - Document test cases
   - Explain test setup
   - Add usage examples

## Review Checklist

Before submitting PR:
- [ ] Code follows style guide
- [ ] All tests pass
- [ ] Documentation updated
- [ ] No memory leaks
- [ ] Performance acceptable
- [ ] Error handling complete
- [ ] CI checks pass

## Getting Help

1. **Resources**
   - Project documentation
   - Issue tracker
   - Discussion forums
   - Code comments

2. **Contact**
   - Open GitHub issue
   - Join discussions
   - Ask for clarification

## License

By contributing, you agree that your contributions will be licensed under the project's MIT License.