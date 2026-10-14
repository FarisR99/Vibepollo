/** @file Capture rectangle validation tests for the application editor. */
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { validCaptureCrop } from '../utils/captureCrop.ts';

test('capture rectangles accept disabled, full-size, offset, and integer boundary values', () => {
  for (const value of [
    '',
    '  ',
    '0,0,3840,2160',
    '3840,0,3840,2160',
    ' 0,0,1,1 ',
    '2147483647,0,1,1',
  ]) {
    assert.equal(validCaptureCrop(value), true, value);
  }
});

test('capture rectangles reject malformed, overflowing, negative, and zero-area values', () => {
  for (const value of [
    '0,0,1',
    '0,0,1,1,2',
    '0,0,0,1',
    '0,0,1,0',
    '-1,0,1,1',
    '0,0,-1,1',
    '+1,0,1,1',
    '0, 0,1,1',
    '0,0,1.5,1',
    '0,0,1,',
    ',0,1,1',
    '2147483648,0,1,1',
    '0,0,999999999999999999999999,1',
  ]) {
    assert.equal(validCaptureCrop(value), false, value);
  }
});
