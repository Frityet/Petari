"""Offline controller math checks; no game process or state mutations."""
import math
import unittest

from follow_actor import controls, raw_stick_for_direction


def original_processed_axes(x, y):
    # Independent scalar transcription of inputStick + calcWorldPadDir's
    # ordinary 3D path. Double transcendental math approximates the original
    # lookup table; quantize its final sin/cos index explicitly.
    x, y = max(-1, min(1, 1.5 * x)), max(-1, min(1, 1.5 * y))
    radius = min(1, math.hypot(x, y))
    angle = math.atan2(y, x) % (2 * math.pi)
    quarter, within = divmod(angle, math.pi / 2)
    if within <= 0.1:
        within = 0
    elif within >= math.pi / 2 - 0.1:
        within = math.pi / 2
    else:
        within = (within - 0.1) * (math.pi / 2) / (math.pi / 2 - 0.2)
    angle = quarter * math.pi / 2 + within
    angle = int(angle * 2607.5945) % 16384 * (2 * math.pi / 16384)
    x, y = radius * math.cos(angle), radius * math.sin(angle)
    if abs(y) > 0.5:
        x = 0 if abs(x) < 0.25 else math.copysign((abs(x) - 0.25) / 0.75, x)
    elif abs(x) > 0.5:
        y = 0 if abs(y) < 0.2 else math.copysign((abs(y) - 0.2) / 0.8, y)
    return x, y


def angular_error(x, y, angle):
    return abs(math.remainder(math.atan2(y, x) - angle, 2 * math.pi))


class ControllerMappingTests(unittest.TestCase):
    def test_known_raw_axis_distortion(self):
        wanted = math.radians(10)
        old = original_processed_axes(math.cos(wanted), math.sin(wanted))
        raw = raw_stick_for_direction(math.cos(wanted), math.sin(wanted), 1)
        new = original_processed_axes(*raw)
        self.assertGreater(math.degrees(angular_error(*old, wanted)), 5)
        self.assertLess(math.degrees(angular_error(*new, wanted)), 0.1)

    def test_quadrants_and_partial_magnitudes(self):
        errors = []
        old_errors = []
        for magnitude in (0.35, 0.5, 0.75, 1.0):
            for degree in range(360):
                angle = math.radians(degree)
                x, y = math.cos(angle), math.sin(angle)
                raw = raw_stick_for_direction(x, y, magnitude)
                self.assertAlmostEqual(math.hypot(*raw), magnitude / 1.5, places=12)
                # These are the exact six-place values sent by the operator.
                actual = original_processed_axes(*(float(f"{axis:.6f}") for axis in raw))
                errors.append(math.degrees(angular_error(*actual, angle)))
                old_errors.append(math.degrees(angular_error(*original_processed_axes(x * magnitude, y * magnitude), angle)))
        # Threshold branches leave genuinely unreachable angles. Do not assert
        # a perfect inverse through those source-defined discontinuities.
        # At magnitude .75 the original branch change around y=.5 leaves
        # roughly a 16-degree output gap. The nearer endpoint can be 8.3
        # degrees away. This is the authored input transfer, not solver error.
        self.assertLess(max(errors), 8.3)
        self.assertLess(sum(errors), sum(old_errors) / 5)
        self.assertLess(sorted(errors)[len(errors) // 2], 0.1)

    def test_controls_maps_normal_camera_and_stop(self):
        player = {"position": [0, 0, 0], "player": {
            "movement_up": [0, 1, 0], "camera_x": [1, 0, 0],
            "camera_y": [0, 1, 0], "camera_z": [0, 0, 1]}}
        angle = math.radians(20)
        target = {"position": [-1000 * math.cos(angle), 0, -1000 * math.sin(angle)]}
        x, y, distance = controls(player, target)
        px, py = original_processed_axes(x, y)
        self.assertAlmostEqual(distance, 1000)
        self.assertLess(math.degrees(angular_error(px, py, angle)), 0.1)
        self.assertEqual(controls(player, {"position": [1, 0, 0]})[:2], (0, 0))

    def test_full_speed_chase_preserves_navigation_slowdown(self):
        player = {"position": [0, 0, 0], "player": {
            "movement_up": [0, 1, 0], "camera_x": [1, 0, 0],
            "camera_y": [0, 1, 0], "camera_z": [0, 0, 1]}}
        target = {"position": [-175, 0, 0]}
        slow_x, slow_y, _ = controls(player, target)
        fast_x, fast_y, _ = controls(player, target, 0, full_speed=True)
        # Cardinal input has no subsequent component dead-zone shrinkage,
        # so independent input shaping exposes the requested magnitudes.
        self.assertAlmostEqual(math.hypot(*original_processed_axes(slow_x, slow_y)), 0.5)
        self.assertAlmostEqual(math.hypot(*original_processed_axes(fast_x, fast_y)), 1.0)
        self.assertEqual(controls(player, {"position": [-100, 0, 0]})[:2], (0, 0))
        self.assertEqual(controls(player, player, 0, full_speed=True)[:2], (0, 0))


if __name__ == "__main__":
    unittest.main()
