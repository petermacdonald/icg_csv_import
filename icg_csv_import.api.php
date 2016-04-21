<?php
/**
 * @file
 * This file documents all available hook functions to manipulate data.
 */

/**
 * Perform a binary fetch of a named file from storage.  This example performs
 * a simple FTP transfer from a network storage server.
 *
 * @param string $path
 *   The path to the file.
 *
 * @return string 
 *   Path to a temporary, local copy of the target file.
 */
function hook_fetch_OBJ($path) {

  static $ftp_username = 'mcfatem';   // @TODO...credentials should be parameters
  static $ftp_userpass = '*******';   // @TODO...update or REMOVE this! 

  $module_name = basename(__FILE__, '.module');

  // Explode the $path... should be of the form server:/dir1/dir2/dir3/filename.ext
  list($server, $rest) = explode(':', $path, 2);
  $parts = pathinfo($rest);
  $ftp_directory = $parts['dirname'];
  $filename = $parts['basename'];

  // Set up a basic connection.
  $conn_id = ftp_ssl_connect($server, 21, 180);
  if (!$conn_id) {
    watchdog($module_name, "FTP connection to server '%server' failed!", array('server' => $server), WATCHDOG_ERROR);
    return FALSE;
  }

  // Login with the specified FTP username and password.
  $login_result = ftp_login($conn_id, $ftp_username, $ftp_userpass);
  if (!$login_result) {
    watchdog($module_name, "FTP login failed!", array( ), WATCHDOG_ERROR);
    return FALSE;
  }

  // Set FTP to passive mode.
  ftp_pasv($conn_id, TRUE);

  // Attempt to change the remote directory as specified.
  $change_dir = ftp_chdir($conn_id, $ftp_directory);
  if (!$change_dir) {
    watchdog($module_name, "FTP chdir to '%directory' failed!", array('%directory' => $ftp_directory), WATCHDOG_ERROR);
    die("ftp_chdir failed!");
    return FALSE;
  } else {
    $txt = "Current directory is: " . ftp_pwd($conn_id);
    watchdog($module_name, $txt, array( ), WATCHDOG_INFO);
  }

  // Fetch a file via FTP.
  $temp_file = drupal_tempnam('temporary://', 'import_content_');   // open a temp file
  if (!ftp_get($conn_id, $temp_file, $filename, FTP_BINARY)) {
    watchdog($module_name, "Could not download file '%filename' via FTP.", array('%filename' => $filename), WATCHDOG_ERROR);
    return FALSE;
  }

  return $temp_file;
}
