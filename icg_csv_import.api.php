<?php
/**
 * @file
 * This file documents all available hook functions to manipulate data.
 */

/**
 * If the MODS xpaths are already in the CSV file and XPath mapping is skipped...
 * call any/all registered hook_fetch_CSV_defaults functions.
 *
 * @return array $return
 *   'label_field' => Specifies the MODS field used to define an object's LABEL.  Usually /mods/titleInfo/title.
 *   'transform' => Specifies the full path of the MODS-to-DC transform to run on each object MODS record.
 */
function hook_fetch_CSV_defaults( ) {
  $values = array(
    'label_field' => '/mods/titleInfo/title',
    'transform' => '/tmp/mods-to-dc.xslt',
  );
  return $values;
}


/**
 * Perform a binary fetch of a named file from storage.  This example performs
 * a simple FTP transfer from a network storage server.
 *
 * @param string $path
 *   The path to the file.
 * @param array $credentials
 *   'username' => The username to use for successful file transfer.
 *   'password' => The corresponding password for file transfer.
 *
 * @return string 
 *   Path to a temporary, local copy of the target file.
 */
function hook_fetch_OBJ($path, $credentials) {

  $ftp_username = $credentials['username'];
  $ftp_userpass = $credentials['password'];

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
