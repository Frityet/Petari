import json
import math
from pathlib import Path
import tempfile
import unittest

from analyze_matrices import analyze, angle, camera_stable_intervals, matrix_stats, quaternion_matrix


class MatrixAnalysisTests(unittest.TestCase):
    def test_translated_quarter_turn_uses_columns_as_world_axes(self):
        m = [[0., -1., 0., 100.], [1., 0., 0., -200.], [0., 0., 1., 300.]]
        stats = matrix_stats(m)
        self.assertEqual(stats["up"], [-1., 0., 0.])
        self.assertEqual(stats["translation"], [100., -200., 300.])
        self.assertEqual(stats["determinant"], 1.)
        self.assertEqual(angle(stats["up"], [0., 1., 0.]), 90.)

    def test_scale_and_reflection_are_distinct_from_shear(self):
        m = [[-2., 0., 0., 0.], [0., 3., 0., 0.], [0., 0., 4., 0.]]
        stats = matrix_stats(m)
        self.assertEqual(stats["axis_lengths"], [2., 3., 4.])
        self.assertEqual(stats["determinant"], -24.)
        self.assertEqual(stats["normalized_determinant"], -1.)
        self.assertEqual(stats["max_normalized_axis_dot"], 0.)
        m[0][1] = 1.
        self.assertGreater(matrix_stats(m)["max_normalized_axis_dot"], .3)

    def test_absent_nonfinite_and_degenerate_are_not_clean_rotations(self):
        self.assertFalse(matrix_stats(None)["present"])
        self.assertFalse(matrix_stats([[float("nan"), 0, 0, 0]] * 3)["finite"])
        self.assertFalse(matrix_stats([[None, 0, 0, 0]] * 3)["finite"])
        stats = matrix_stats([[0., 0., 0., 0.]] * 3)
        self.assertEqual(stats["determinant"], 0.)
        self.assertIsNone(stats["normalized_determinant"])
        self.assertIsNone(angle([0, 0, 0], [0, 1, 0]))

    def test_quaternion_reference_rotates_up_and_ignores_global_sign(self):
        a = math.sqrt(.5)
        q = [0., 0., a, a]
        m = quaternion_matrix(q)
        self.assertLess(angle(matrix_stats(m)["up"], [-1., 0., 0.]), 1e-5)
        self.assertEqual(m, quaternion_matrix([-x for x in q]))

    def test_live_read_excludes_only_incomplete_tail(self):
        record = {"frame_index": 50, "actors": []}
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "trace.jsonl"
            prefix = json.dumps(record).encode() + b"\n"
            path.write_bytes(prefix + b'{"frame_index":')
            summary, rows = analyze(path)
            self.assertEqual(summary["last_frame"], 50)
            self.assertEqual(summary["ignored_incomplete_tail_bytes"], 15)
            self.assertEqual(rows, [])
            path.write_bytes(prefix + b'{"frame_index":\n')
            with self.assertRaises(json.JSONDecodeError):
                analyze(path)

    def test_frame_window_keeps_file_extent_separate(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "trace.jsonl"
            path.write_text("".join(json.dumps({"frame_index": f, "actors": []}) + "\n"
                                    for f in (10, 20, 30)))
            summary, _ = analyze(path, 15, 25)
            self.assertEqual(summary["last_frame"], 30)
            self.assertEqual(summary["selection"]["first_selected_frame"], 20)
            self.assertEqual(summary["selection"]["last_selected_frame"], 20)

    def test_stable_head_intervals_expose_reset_and_countdown_without_merging_motion(self):
        def row(frame, head, timer):
            return {"id": 1, "frame": frame, "head": head, "camera_up_timer": timer,
                    "status": 0, "state_type": "", "camera_up_vs_head_degrees": 2.}
        samples = [row(0, [0, 1, 0], 20), row(10, [0, 1, 0], 20),
                   row(20, [0, 1, 0], 10), row(30, [1, 0, 0], 20)]
        result = camera_stable_intervals(samples)
        self.assertEqual(len(result), 2)
        self.assertEqual((result[0]["timer_before"], result[0]["timer_after"]), (20, 20))
        self.assertEqual((result[1]["timer_before"], result[1]["timer_after"]), (20, 10))


if __name__ == "__main__":
    unittest.main()
