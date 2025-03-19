# Menu Extension Test Plan

## Overview

This document outlines the testing strategy for the menu extension system. The testing approach focuses on unit tests for individual components, integration tests for component interactions, and system tests for end-to-end validation.

## Test Framework Structure

```
tests/
├── unit/
│   ├── menu_manager_tests.c
│   ├── menu_tests.c
│   ├── navigation_tests.c
│   └── activation_tests.c
├── integration/
│   ├── input_handling_tests.c
│   ├── menu_lifecycle_tests.c
│   └── rendering_tests.c
└── system/
    └── end_to_end_tests.c
```

## Unit Tests

### 1. Menu Manager Tests

```c
void test_menu_manager() {
    // Creation/Destruction
    test_menu_manager_create();
    test_menu_manager_destroy();
    
    // Registration
    test_register_menu();
    test_register_multiple_menus();
    test_register_duplicate_menu();
    
    // Activation
    test_activate_menu();
    test_activate_with_invalid_keys();
    test_activate_while_other_active();
    
    // Deactivation
    test_deactivate_menu();
    test_deactivate_inactive_menu();
}
```

### 2. Menu Component Tests

```c
void test_menu() {
    // Configuration
    test_menu_config_validation();
    test_menu_config_defaults();
    
    // State Management
    test_menu_state_transitions();
    test_menu_visibility();
    test_menu_selection();
    
    // Item Management
    test_add_menu_items();
    test_remove_menu_items();
    test_update_menu_items();
}
```

### 3. Navigation Tests

```c
void test_navigation() {
    // Basic Navigation
    test_next_item_selection();
    test_prev_item_selection();
    test_wrap_around_navigation();
    
    // Direct Selection
    test_direct_key_selection();
    test_invalid_direct_key();
    
    // Custom Navigation
    test_custom_navigation_callbacks();
    test_navigation_extension_data();
}
```

### 4. Activation Tests

```c
void test_activation() {
    // Mod Release Activation
    test_mod_release_activation();
    test_mod_release_disabled();
    
    // Direct Key Activation
    test_direct_key_activation();
    test_direct_key_disabled();
    
    // Custom Activation
    test_custom_activation_callback();
    test_activation_with_data();
}
```

## Integration Tests

### 1. Input Handling Integration

```c
void test_input_handling() {
    // Key Event Processing
    test_key_press_chain();
    test_key_release_chain();
    test_modifier_tracking();
    
    // Menu Activation Flow
    test_mod_key_activation_flow();
    test_direct_key_activation_flow();
    
    // Navigation Integration
    test_navigation_with_input();
    test_selection_with_input();
}
```

### 2. Menu Lifecycle Integration

```c
void test_menu_lifecycle() {
    // State Transitions
    test_inactive_to_active_flow();
    test_active_to_inactive_flow();
    test_navigation_state_flow();
    
    // Resource Management
    test_renderer_lifecycle();
    test_input_handler_lifecycle();
    
    // Error Conditions
    test_invalid_state_transitions();
    test_resource_cleanup();
}
```

### 3. Rendering Integration

```c
void test_rendering() {
    // Basic Rendering
    test_initial_render();
    test_rerender_on_selection();
    test_rerender_on_update();
    
    // Style Application
    test_style_changes();
    test_custom_style_callbacks();
    
    // Performance
    test_render_performance();
    test_batch_updates();
}
```

## System Tests

### 1. End-to-End Scenarios

```c
void test_end_to_end() {
    // Window Switcher Menu
    test_window_switcher_workflow();
    test_window_activation();
    
    // Multiple Menu Interaction
    test_menu_switching();
    test_menu_cleanup();
    
    // Error Recovery
    test_x11_error_recovery();
    test_resource_exhaustion();
}
```

## Test Utilities

```c
// Mock X11 environment
typedef struct {
    xcb_connection_t* conn;
    xcb_window_t root;
    xcb_screen_t* screen;
} MockX11Environment;

// Input simulation
typedef struct {
    uint16_t modifiers;
    uint8_t keycode;
    xcb_timestamp_t time;
} MockKeyEvent;

// Test assertions
#define ASSERT_MENU_STATE(menu, expected_state) // ...
#define ASSERT_SELECTION(menu, expected_index) // ...
#define ASSERT_VISIBILITY(menu, expected_visible) // ...
```

## Test Coverage Goals

1. **Line Coverage**: Minimum 90% coverage for core components
2. **Branch Coverage**: Minimum 85% coverage for decision points
3. **Path Coverage**: Key workflows fully covered
4. **Function Coverage**: All public APIs tested

## Testing Guidelines

1. **Setup/Teardown**
   - Each test must clean up resources
   - Use fixture for common setup
   - Isolate X11 dependencies

2. **Assertions**
   - Verify state changes
   - Check resource cleanup
   - Validate error conditions

3. **Mocking**
   - Mock X11 connections
   - Simulate input events
   - Mock rendering context

4. **Performance**
   - Measure render times
   - Track memory usage
   - Profile key operations

## Continuous Integration

1. **Build Pipeline**
   - Compile with warnings as errors
   - Run unit tests
   - Run integration tests
   - Run system tests
   - Check coverage

2. **Performance Testing**
   - Benchmark key operations
   - Compare against baselines
   - Alert on regressions

3. **Static Analysis**
   - Run clang-tidy
   - Check memory leaks
   - Validate thread safety

## Test Execution

```bash
# Run all tests
make test

# Run specific test suite
make test-unit
make test-integration
make test-system

# Run with coverage
make test-coverage

# Run with sanitizers
make test-asan
make test-ubsan