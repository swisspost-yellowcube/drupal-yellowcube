<?php
/**
 * @file
 * Contains the administrative pages and form callbacks for the YellowCube module.
 */


/**
 * Builds the form for configuring the YellowCube integration.
 */
function yellowcube_admin_settings_form($form, &$form_state) {

  $form['yellowcube_sender'] = array(
    '#type' => 'textfield',
    '#title' => t('YellowCube depositor ID'),
    '#description' => t('ID number assigned to your business by YellowCube.'),
    '#default_value' => variable_get('yellowcube_sender', ''),
  );

  $form['yellowcube_operating_mode'] = array(
    '#type' => 'select',
    '#title' => t('Operating mode'),
    '#description' => t('Select operating mode, test or production.'),
    '#options' => array(
      YELLOWCUBE_OPERATING_MODE_TEST => t('Test'),
      YELLOWCUBE_OPERATING_MODE_PROD => t('Production'),
    ),
    '#default_value' => variable_get('yellowcube_operating_mode', YELLOWCUBE_OPERATING_MODE_TEST),
  );

  return system_settings_form($form);
}
