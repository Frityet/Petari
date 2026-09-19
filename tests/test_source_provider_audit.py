#!/usr/bin/env python3
"""Mutation and incomplete-evidence regression checks for provider reporting."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('audit', ROOT / 'scripts/source_provider_audit.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class ProviderAuditTests(unittest.TestCase):
    def test_relocated_original_mutation_is_not_hidden_by_directory(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'compat').mkdir()
            (root / 'original.cpp').write_text('float ease(float x) { return table(x); }')
            provider = root / 'compat/ease.cpp'
            provider.write_text('float ease(float x) { /* import */ return table(x); }')
            records = [dict(original='original.cpp', provider='compat/ease.cpp', anchor='float ease(')]
            self.assertEqual(audit.check_provenance(root, records)[0]['status'], 'source-tokens-exact')
            provider.write_text('float ease(float x) { return cos(x); }')
            self.assertEqual(audit.check_provenance(root, records)[0]['status'], 'source-tokens-differ')
            provider.unlink()
            self.assertEqual(audit.check_provenance(root, records)[0]['status'], 'unresolved')

    def test_literals_comments_and_overload_ambiguity(self):
        source = '''int f() { const char* s = "} // x"; /* } */ return 1; }
                    int other() { return 9; }'''
        self.assertEqual(audit.definition(source, 'int f(')[-4:], ['return', '1', ';', '}'])
        with self.assertRaises(ValueError):
            audit.definition(source + ' int f(int x) { return x; }', 'int f(')
        with self.assertRaises(ValueError):
            audit.definition('int f();', 'int f(')
        self.assertNotEqual(audit.tokens('return 1 + +x;'), audit.tokens('return 1 ++x;'))

    def test_archive_duplicate_retained_even_when_executable_presence_unknown(self):
        rows = audit.parse_nm('game.a[Original.cpp.o]: _fn T 0 0\ngame.a[Compat.cpp.o]: _fn T 0 0\n')
        self.assertEqual(len(audit.duplicates(rows)['_fn']), 2)
        with self.assertRaises(ValueError):
            audit.parse_nm('an unexpected tool diagnostic')

    def test_new_overload_does_not_inherit_reviewed_body_approval(self):
        records = [dict(provider='compat.cpp', abi_symbol='_ZN2MR5queryEi', status='source-tokens-exact')]
        self.assertEqual(audit.symbol_provenance(records, 'compat.cpp', '__ZN2MR5queryEi'), 'source-tokens-exact')
        self.assertEqual(audit.symbol_provenance(records, 'compat.cpp', '_ZN2MR5queryEf'), 'unreviewed')
        self.assertEqual(audit.symbol_provenance(records, 'another.cpp', '_ZN2MR5queryEi'), 'unreviewed')

    def test_reviewed_body_without_its_exported_owner_is_incomplete(self):
        records = [dict(provider='compat.cpp', abi_symbol='_ZN2MR5queryEi')]
        rows = [dict(sources='compat.cpp', symbol='__ZN2MR5queryEi', mapping='unique')]
        self.assertEqual(audit.missing_reviewed_symbols(records, rows), [])
        rows[0]['sources'] = 'unreviewed.cpp'
        self.assertEqual(audit.missing_reviewed_symbols(records, rows), records)

    def test_reviewed_imports_currently_match_without_approving_other_sources(self):
        records = json.loads((ROOT / 'scripts/source_provider_provenance.json').read_text())
        result = audit.check_provenance(ROOT, records)
        self.assertTrue(result)
        self.assertEqual({r['status'] for r in result}, {'source-tokens-exact'}, result)
        # The manifest is explicitly partial; neither absence nor filename grants approval.
        self.assertEqual(audit.check_provenance(ROOT, []), [])


if __name__ == '__main__':
    unittest.main()
