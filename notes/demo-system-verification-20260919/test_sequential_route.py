"""Offline state-machine, trace-tail and publication tests; no game/GPU launch."""
import io
import argparse
import fcntl
import subprocess
import sys
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from operate_first_catch import Operator, StallJump, available, extra_button_script, load_plan, publish

HERE = Path(__file__).resolve().parent
PLAN = load_plan(HERE / "three-rabbit-route.json")


def actor(identity, kind, nerve, position=(0, 0, 0), dead=False, hidden=False):
    return {"id": identity, "type": kind, "nerve": {"type": "Nrv" + nerve + "E"},
            "position": list(position), "dead": dead, "hidden": hidden}


def fixture():
    rabbits = [actor(5000+i, "13RunawayRabbit", "NoActive", item["rabbit_spawn"])
               for i, item in enumerate(PLAN["route"])]
    rabbits.append(actor(902, "13RunawayRabbit", "NoActive", (0, 0, 0)))
    guide = actor(12, "10DemoRabbit", "Talk0")
    collector = actor(13, "20RunawayRabbitCollect", "Wait")
    player = actor(1, "Mario", "Wait")
    player['player'] = {"movement_up": [0, 1, 0], "camera_x": [1, 0, 0],
                        "camera_y": [0, 1, 0], "camera_z": [0, 0, 1]}
    op = Operator(PLAN['route'], 1400, 30000, 1400, True)
    actors = rabbits + [guide, collector, player]
    op.observe({"frame_index": 1400, "actors": actors})
    collector['nerve']['type'] = 'NrvActiveE'
    op.observe({"frame_index": 1500, "actors": actors})
    return op, actors, rabbits


class RouteTests(unittest.TestCase):
    def test_only_chase_requests_full_speed(self):
        op, actors, rabbits = fixture()
        snap = {'frame_index': 2000, 'actors': actors}
        with patch('operate_first_catch.controls', return_value=(0, 0, 175)) as control:
            op.decision(snap)
            self.assertEqual(op.phase, 'reveal')
            self.assertFalse(control.call_args.kwargs['full_speed'])
            rabbits[0]['nerve']['type'] = 'NrvRunawayE'
            op.observe(snap)
            op.decision(snap)
            self.assertEqual(op.phase, 'chase')
            self.assertTrue(control.call_args.kwargs['full_speed'])

    def test_binds_current_ids_once(self):
        op, actors, rabbits = fixture()
        self.assertEqual(op.rabbit_ids, [5000, 5001, 5002])
        rabbits[0]['position'] = [9, 8, 7]
        op.observe({'frame_index': 1510, 'actors': actors})
        self.assertEqual(op.rabbit_ids, [5000, 5001, 5002])
        self.assertEqual(op.binding_evidence['frame'], 1400)

    def test_rejects_late_binding(self):
        op, actors, rabbits = fixture()
        op.rabbit_ids = None
        rabbits[0]['nerve']['type'] = 'NrvHideE'
        with self.assertRaisesRegex(RuntimeError, 'NoActive'):
            op.observe({'frame_index': 2000, 'actors': actors})

    def test_match_is_strictly_under_ten(self):
        op, actors, rabbits = fixture()
        op.rabbit_ids = None
        rabbits[0]['position'][0] += 10
        with self.assertRaisesRegex(RuntimeError, 'within 10'):
            op.observe({'frame_index': 1400, 'actors': actors})

    def test_ambiguous_spawn_rejected(self):
        op, actors, rabbits = fixture()
        op.rabbit_ids = None
        actors.append(actor(8000, '13RunawayRabbit', 'NoActive', rabbits[0]['position']))
        with self.assertRaisesRegex(RuntimeError, 'Ambiguous'):
            op.observe({'frame_index': 1400, 'actors': actors})

    def test_all_three_dialogues_then_visible_rosetta(self):
        op, actors, rabbits = fixture()
        for index in range(3):
            frame = 2000 + index * 4000
            rabbit = rabbits[index]
            rabbit['nerve']['type'] = 'NrvRunawayE'
            op.observe({'frame_index': frame, 'actors': actors})
            self.assertEqual(op.phase, 'chase')
            rabbit['nerve']['type'] = 'NrvCaughtTalkE'
            op.observe({'frame_index': frame+10, 'actors': actors})
            rabbit['dead'] = True
            op.observe({'frame_index': frame+900, 'actors': actors})
            self.assertEqual(op.phase, 'caught')  # Old age>=240 condition would stop.
            self.assertEqual(op.index, index)
            tico = actor(6000+index, '11RunawayTico', 'Talk')
            actors.append(tico)
            op.observe({'frame_index': frame+910, 'actors': actors})
            self.assertEqual(op.tico_id, tico['id'])
            actors.remove(tico)
            op.observe({'frame_index': frame+920, 'actors': actors})
            self.assertEqual(op.phase, 'caught')  # Absence is not completion.
            actors.append(tico)
            tico['nerve']['type'] = 'NrvWhiteOutE' if index == 2 else 'NrvWaitE'
            op.observe({'frame_index': frame+930, 'actors': actors})
            self.assertEqual(op.index, index+1)
        self.assertEqual(op.phase, 'await_rosetta')
        self.assertIsNone(op.finished_reason)
        rosetta = actor(9000, '7Rosetta', 'Wait', dead=True)
        actors.append(rosetta)
        for dead, hidden in [(True, False), (False, True)]:
            rosetta.update(dead=dead, hidden=hidden)
            snapshot = {'frame_index': 12000, 'actors': actors}
            op.observe(snapshot)
            self.assertIsNone(op.finished_reason)
            self.assertEqual(op.decision(snapshot)['command']['buttons'], '')
            self.assertIn(':0.000000:0.000000', op.decision(snapshot)['command']['stick'])
        rosetta.update(dead=False, hidden=False)
        op.observe({'frame_index': 12010, 'actors': actors})
        self.assertEqual(op.finished_reason, 'rosetta_alive_not_hidden')
        self.assertEqual(op.rosetta_id, 9000)
        self.assertEqual(len(op.completed), 3)

    def test_no_press_during_navigation(self):
        op, actors, rabbits = fixture()
        snap = {'frame_index': 2000, 'actors': actors}
        self.assertEqual(op.decision(snap)['command']['buttons'], '')
        rabbits[0]['nerve']['type'] = 'NrvRunawayE'
        op.observe(snap)
        self.assertEqual(op.decision(snap)['command']['buttons'], '')

    def test_frame_limit_is_not_success(self):
        op, actors, _ = fixture()
        op.observe({'frame_index': 30000, 'actors': actors})
        self.assertEqual(op.finished_reason, 'frame_limit')
        self.assertEqual(op.completed, [])

    def test_single_waypoint_preserved(self):
        op = Operator([{'label': 'single', 'waypoint': [0, 0, 0]}], 1400, 9000, rabbit_id=99)
        actors = [actor(13, '20RunawayRabbitCollect', 'Active'), actor(99, '13RunawayRabbit', 'Runaway')]
        op.observe({'frame_index': 1400, 'actors': actors})
        self.assertEqual(op.phase, 'chase')
        self.assertEqual(op.rabbit_ids, [99])

    def test_tail_preserves_partial_and_every_transition(self):
        source = io.StringIO('{"frame_index":1}\n{"frame_index":2')
        self.assertEqual(list(available(source)), [{'frame_index': 1}])
        self.assertEqual(source.read(), '{"frame_index":2')
        source = io.StringIO('{"frame_index":1}\n{"frame_index":2}\n')
        self.assertEqual([s['frame_index'] for s in available(source)], [1, 2])

    def test_publish_atomic_unique_temp_and_neutral(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/'input.json'
            for command in [{'buttons': '10-20:A', 'pointer': '', 'stick': ''},
                            {'buttons': '', 'pointer': '', 'stick': ''}]:
                publish(path, command)
                self.assertEqual(json.loads(path.read_text()), command)
                self.assertEqual(list(Path(folder).iterdir()), [path])

    def test_existing_input_owner_prevents_all_writes(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/'input.json'
            path.write_text('existing owner input\n')
            lock = path.with_name(path.name + '.operator-lock')
            with lock.open('a') as owner:
                fcntl.flock(owner, fcntl.LOCK_EX | fcntl.LOCK_NB)
                child = subprocess.run([sys.executable, str(HERE/'operate_first_catch.py'),
                                        str(Path(folder)/'absent-trace.jsonl'), str(path),
                                        '--waypoint', '0', '0', '0', '--timeout', '1'],
                                       capture_output=True, text=True, timeout=3)
                self.assertNotEqual(child.returncode, 0)
                self.assertIn('BlockingIOError', child.stderr)
                self.assertEqual(path.read_text(), 'existing owner input\n')

    def test_explicit_buttons_merge_and_finished_neutral(self):
        op, actors, _ = fixture()
        op.extra_buttons = extra_button_script('15000-15025:A;15100:Z+B')
        snapshot = {'frame_index': 2000, 'actors': actors}
        self.assertEqual(op.decision(snapshot)['command']['buttons'], op.extra_buttons)
        op.phase = 'caught'
        op.next_press = 2000
        self.assertEqual(op.decision(snapshot)['command']['buttons'], '2001-2010:A;' + op.extra_buttons)
        op.finished_reason = 'frame_limit'
        self.assertEqual(op.decision(snapshot)['command']['buttons'], '')

    def test_extra_buttons_preserve_native_parser_input(self):
        for text in ['', '15000-15025:A', '10:A+B; 20- : ONE + TWO', ' ;0:C;',
                     '0-18446744073709551615:PLUS+MINUS+HOME+UP+DOWN+LEFT+RIGHT+Z+C+1+2']:
            self.assertEqual(extra_button_script(text), text)
        for text in [None, 123, []]:
            with self.assertRaises(argparse.ArgumentTypeError):
                extra_button_script(text)

    def test_extra_buttons_reject_invalid_native_syntax(self):
        for text in ['23350-23380:A,27040-27070:A', '10:A|B', '10:A+', '10:+',
                     '10:A++B', '10:a', '10:BOGUS', '10:', '10', ':A',
                     '-10:A', '+10:A', '20-10:A', '1-2-3:A', '1 2:A',
                     '18446744073709551616:A', '0-18446744073709551616:A', '１:A']:
            with self.subTest(text=text), self.assertRaises(argparse.ArgumentTypeError):
                extra_button_script(text)

    def test_bad_button_cli_never_publishes(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'input.json'
            path.write_text('previous valid controller state\n')
            child = subprocess.run([sys.executable, str(HERE / 'operate_first_catch.py'),
                                    str(Path(folder) / 'absent-trace.jsonl'), str(path),
                                    '--waypoint', '0', '0', '0', '--timeout', '1',
                                    '--extra-buttons', '23350-23380:A,27040-27070:A'],
                                   capture_output=True, text=True, timeout=3)
            self.assertEqual(child.returncode, 2)
            self.assertIn('extra buttons', child.stderr)
            self.assertEqual(path.read_text(), 'previous valid controller state\n')

    def test_stall_jump_window_pulse_and_cooldown(self):
        jump = StallJump()
        for frame in range(0, 240, 10):
            self.assertEqual(jump.update(frame, [frame % 3, 0, 0], 'chase', True, True), ('', None))
        script, event = jump.update(240, [0, 0, 0], 'chase', True, True)
        self.assertEqual(script, '241-270:A')
        self.assertEqual(event['observed_first_frame'], 0)
        self.assertLess(event['position_bounds_diagonal'], 20)
        self.assertEqual(jump.update(250, [0, 2, 0], 'chase', True, False)[0], script)
        for frame in range(280, 780, 10):
            self.assertEqual(jump.update(frame, [0, 0, 0], 'chase', True, True), ('', None))
        self.assertEqual(jump.update(780, [0, 0, 0], 'chase', True, True)[0], '781-810:A')

    def test_stall_jump_does_not_mistake_loops_or_missing_samples(self):
        jump = StallJump()
        for frame in range(0, 1000, 10):
            position = [100 * (frame % 240) / 240, 0, 0]
            self.assertEqual(jump.update(frame, position, 'chase', True, True), ('', None))
        jump = StallJump()
        jump.update(0, [0, 0, 0], 'chase', True, True)
        self.assertEqual(jump.update(1000, [0, 0, 0], 'chase', True, True), ('', None))

    def test_stall_jump_requires_ordinary_accepted_input(self):
        for blocked in ('opening', 'caught', 'guide', 'dialogue', 'pipe', 'input_gated', 'air', 'special_status'):
            with self.subTest(blocked=blocked):
                op, actors, rabbits = fixture()
                player = next(a for a in actors if 'player' in a)
                player['player'].update(status=0, ground_triangle={'host_id': 99},
                                        stick_position=[1, 0, 1], world_pad_direction=[1, 0, 0])
                if blocked in ('opening', 'caught', 'guide', 'dialogue'):
                    op.phase = blocked
                elif blocked == 'pipe':
                    actors.append(actor(88, '11EarthenPipe', 'PlayerIn'))
                elif blocked == 'input_gated':
                    player['player']['stick_position'] = [0, 0, 0]
                elif blocked == 'air':
                    player['player']['ground_triangle'] = None
                else:
                    player['player']['status'] = 1
                for frame in range(2000, 2500, 10):
                    result = op.decision({'frame_index': frame, 'actors': actors})
                    self.assertIsNone(result['automatic_jump'])

    def test_stall_jump_is_logged_and_cancelled_by_pipe(self):
        op, actors, _ = fixture()
        player = next(a for a in actors if 'player' in a)
        player['player'].update(status=0, ground_triangle={'host_id': 99},
                                stick_position=[1, 0, 1], world_pad_direction=[1, 0, 0])
        for frame in range(2000, 2250, 10):
            result = op.decision({'frame_index': frame, 'actors': actors})
        self.assertEqual(result['command']['buttons'], '2241-2270:A')
        self.assertEqual(result['automatic_jump']['observed_last_frame'], 2240)
        actors.append(actor(88, '11EarthenPipe', 'Ready'))
        result = op.decision({'frame_index': 2250, 'actors': actors})
        self.assertIsNone(result['automatic_jump'])
        self.assertEqual(result['command']['buttons'], '')

    def test_invalid_plan(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/'plan.json'
            for plan in [{'version': 1, 'route': []}, {'version': 2},
                         {'version': 1, 'route': [{'label': 'x', 'waypoint': [0, 0, float('nan')], 'rabbit_spawn': [0, 0, 0]}]}]:
                path.write_text(json.dumps(plan))
                with self.assertRaises(ValueError): load_plan(path)


if __name__ == '__main__':
    unittest.main()
