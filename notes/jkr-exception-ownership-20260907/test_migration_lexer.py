import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('migration', Path(__file__).with_name('migrate_native_exceptions.py'))
migration = importlib.util.module_from_spec(spec)
spec.loader.exec_module(migration)


class MigrationLexerTests(unittest.TestCase):
    def test_only_real_standard_throw_expression_is_selected(self):
        source = r'''// throw std::logic_error("comment");
        const char* text = "throw std::runtime_error(\"literal\")";
        const char* raw = R"tag(throw std::runtime_error(")tag";
        char quote = '\'';
        int separator = 1'000;
        throw CustomError("unchanged");
        throw std::bad_alloc();
        throw std::logic_error(std::string("a,(b)") + [] { return "c"; }());'''
        found = migration.occurrences(source)
        self.assertEqual(len(found), 1)
        self.assertEqual(found[0]['type'], 'std::logic_error')
        self.assertEqual(found[0]['argument'], 'std::string("a,(b)") + [] { return "c"; }()')

    def test_balanced_arguments_preserve_source_exactly(self):
        source = 'throw std::runtime_error(\n  format("[{}]", values[index(2)] /* ) */)\n);'
        [entry] = migration.occurrences(source)
        replaced = source[:entry['start']] + entry['after'] + source[entry['prefix_end']:]
        self.assertEqual(replaced, 'aurora::throw_host_exception<std::runtime_error>(\n  format("[{}]", values[index(2)] /* ) */)\n);')

    def test_multiple_top_level_arguments_stop_for_review(self):
        with self.assertRaises(ValueError):
            migration.occurrences('throw std::runtime_error(a, b);')


if __name__ == '__main__':
    unittest.main()
