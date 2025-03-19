#!/usr/bin/env python3
"""
Test Report Generator for Menu System

Generates detailed HTML reports from test results, including:
- Unit test results
- Integration test results
- Performance benchmarks
- Code coverage
- Memory analysis
"""

import sys
import os
import json
import datetime
import re
from typing import Dict, List, Optional

# Configuration
REPORT_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Menu System Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .section { margin: 20px 0; padding: 20px; border: 1px solid #ddd; }
        .pass { color: green; }
        .fail { color: red; }
        .warn { color: orange; }
        table { border-collapse: collapse; width: 100%; }
        th, td { padding: 8px; text-align: left; border: 1px solid #ddd; }
        th { background-color: #f2f2f2; }
        .metric { font-weight: bold; }
        .chart { width: 100%; height: 300px; }
    </style>
    <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
</head>
<body>
    <h1>Menu System Test Report</h1>
    <div class="section">
        <h2>Summary</h2>
        {summary}
    </div>
    <div class="section">
        <h2>Unit Tests</h2>
        {unit_tests}
    </div>
    <div class="section">
        <h2>Integration Tests</h2>
        {integration_tests}
    </div>
    <div class="section">
        <h2>Performance Benchmarks</h2>
        {performance}
    </div>
    <div class="section">
        <h2>Code Coverage</h2>
        {coverage}
    </div>
    <div class="section">
        <h2>Memory Analysis</h2>
        {memory}
    </div>
</body>
</html>
"""

class TestResult:
    def __init__(self, name: str, passed: bool, duration: float, details: str = ""):
        self.name = name
        self.passed = passed
        self.duration = duration
        self.details = details

class PerformanceMetric:
    def __init__(self, name: str, value: float, unit: str):
        self.name = name
        self.value = value
        self.unit = unit

class CoverageData:
    def __init__(self, file: str, line: float, branch: float, function: float):
        self.file = file
        self.line = line
        self.branch = branch
        self.function = function

def parse_test_output(log_file: str) -> List[TestResult]:
    """Parse test output log file into TestResult objects."""
    results = []
    current_test = None
    
    with open(log_file, 'r') as f:
        for line in f:
            if line.startswith("Running test:"):
                if current_test:
                    results.append(current_test)
                name = line.split(":")[1].strip()
                current_test = TestResult(name, True, 0.0)
            elif "FAILED" in line and current_test:
                current_test.passed = False
                current_test.details += line
            elif "time:" in line and current_test:
                try:
                    current_test.duration = float(line.split(":")[1].strip().split()[0])
                except ValueError:
                    pass
    
    if current_test:
        results.append(current_test)
    
    return results

def parse_performance_output(perf_log: str) -> List[PerformanceMetric]:
    """Parse performance benchmark output."""
    metrics = []
    
    with open(perf_log, 'r') as f:
        for line in f:
            if "Average" in line:
                parts = line.split(":")
                if len(parts) == 2:
                    name = parts[0].strip()
                    try:
                        value = float(parts[1].split()[0])
                        unit = parts[1].split()[1]
                        metrics.append(PerformanceMetric(name, value, unit))
                    except (ValueError, IndexError):
                        pass
    
    return metrics

def parse_coverage(coverage_file: str) -> List[CoverageData]:
    """Parse gcov coverage data."""
    coverage = []
    
    with open(coverage_file, 'r') as f:
        for line in f:
            if "File" in line:
                file = line.split()[-1]
                # Read next three lines for coverage data
                try:
                    lines = float(next(f).split(":")[1].strip().rstrip("%"))
                    branches = float(next(f).split(":")[1].strip().rstrip("%"))
                    functions = float(next(f).split(":")[1].strip().rstrip("%"))
                    coverage.append(CoverageData(file, lines, branches, functions))
                except (StopIteration, ValueError, IndexError):
                    pass
    
    return coverage

def generate_summary(unit_tests: List[TestResult],
                    integration_tests: List[TestResult],
                    performance: List[PerformanceMetric],
                    coverage: List[CoverageData]) -> str:
    """Generate HTML summary section."""
    total_tests = len(unit_tests) + len(integration_tests)
    passed_tests = sum(1 for t in unit_tests + integration_tests if t.passed)
    
    return f"""
    <table>
        <tr>
            <th>Metric</th>
            <th>Value</th>
        </tr>
        <tr>
            <td>Total Tests</td>
            <td>{total_tests}</td>
        </tr>
        <tr>
            <td>Passed Tests</td>
            <td class="{'pass' if passed_tests == total_tests else 'fail'}">
                {passed_tests}/{total_tests}
            </td>
        </tr>
        <tr>
            <td>Average Performance</td>
            <td>{sum(m.value for m in performance)/len(performance):.2f} ms</td>
        </tr>
        <tr>
            <td>Average Coverage</td>
            <td>{sum(c.line for c in coverage)/len(coverage):.1f}%</td>
        </tr>
    </table>
    """

def generate_test_table(tests: List[TestResult], title: str) -> str:
    """Generate HTML table for test results."""
    return f"""
    <h3>{title}</h3>
    <table>
        <tr>
            <th>Test</th>
            <th>Result</th>
            <th>Duration</th>
            <th>Details</th>
        </tr>
        {''.join(f"""
        <tr>
            <td>{t.name}</td>
            <td class="{'pass' if t.passed else 'fail'}">
                {('PASS' if t.passed else 'FAIL')}
            </td>
            <td>{t.duration:.3f}s</td>
            <td>{t.details}</td>
        </tr>
        """ for t in tests)}
    </table>
    """

def main():
    # Parse command line arguments for log files
    if len(sys.argv) < 5:
        print("Usage: generate_report.py <unit_log> <integration_log> "
              "<performance_log> <coverage_log>")
        sys.exit(1)
    
    unit_log = sys.argv[1]
    integration_log = sys.argv[2]
    performance_log = sys.argv[3]
    coverage_log = sys.argv[4]
    
    # Parse log files
    unit_tests = parse_test_output(unit_log)
    integration_tests = parse_test_output(integration_log)
    performance = parse_performance_output(performance_log)
    coverage = parse_coverage(coverage_log)
    
    # Generate report sections
    summary = generate_summary(unit_tests, integration_tests, performance, coverage)
    unit_table = generate_test_table(unit_tests, "Unit Tests")
    integration_table = generate_test_table(integration_tests, "Integration Tests")
    
    # Generate full report
    report = REPORT_TEMPLATE.format(
        summary=summary,
        unit_tests=unit_table,
        integration_tests=integration_table,
        performance=generate_performance_charts(performance),
        coverage=generate_coverage_charts(coverage),
        memory="Memory analysis not available"
    )
    
    # Write report to file
    with open("test_report.html", "w") as f:
        f.write(report)
    
    print("Report generated: test_report.html")

if __name__ == "__main__":
    main()