<?php


/**
 * @file
 * Drush implementation for the cleanup_tool module.
 */

/**
 * Implementats hook_drush_command().
 */
function command_drush_command() {
  $items = array();
  $items['drush-cmd1'] = array(
    'description' => '...',
  );
  $items['drush-cmd2'] = array(
    'arguments' => array(),
    'description' => '...)',
  );
  return $items;
}

/**
 * Drush command callback.
 */
function command_drush_cmd1() {
  drush_print("OK");
}

/**
 * Drush command callback.
 */
function command_drush_cmd2() {
   drush_print("OK");
}
