<?php
/**
 * @file
 * commerce_ogone_direct.inc
 * Functions for Direct integration.
 */

/**
 * Payment method callback: checkout form.
 */
function commerce_ogone_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');

  $card_types = commerce_ogone_direct_all_card_names();
  foreach ($card_types as $key => $value) {
    $cards_id[] = $key;
  }
  $form = commerce_payment_credit_card_form();
  $credit_card_settings = array(
    'type'       => $cards_id,
    'code'       => 'CVV/CV2',
  );

  $form = commerce_payment_credit_card_form($credit_card_settings);
  $form['credit_card']['type']['#options'] = $card_types;

  return $form;
}


/**
 * Get all card types.
 */
function commerce_ogone_direct_all_card_names() {
  return array(
    'visa'    => t('Visa'),
    'mastercard'      => t('Mastercard'),
    'amex'    => t('American Express'),
    'maestro' => t('Maestro'),
    'BC/MC' => t('bancontact/mistercash'),
  );

}

/**
 * Payment method callback: checkout form validation.
 */
function commerce_ogone_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
  // Validate credit card.
  $settings = array(
    'form_parents' => array_merge($form_parents, array('credit_card')),
  );
    if (commerce_payment_credit_card_validate_bc($pane_values['credit_card']) != TRUE and (commerce_payment_credit_card_validate($pane_values['credit_card'], $settings) == FALSE )) {
      return FALSE;
    }

}

/**
 * Validates cards for Bancontact/Mistercash cards.
 */
function commerce_payment_credit_card_validate_bc($card_type) {
  //Check the card number.
  $first_int = intval(substr($card_type['number'], 0, 8));
  $second_int = intval(substr($card_type['number'], 8, 16));
  if (strlen($card_type['number']) == 16 and is_int($first_int) and $first_int > 0 and is_int($second_int) and $second_int > 0) {
    if (substr($first_int, 0, 4) == 6573 and substr($first_int, -4, 8) >= 0300 and substr($first_int, -4, 8) <= 5099) {
       drupal_set_message(t('test card inside'), 'status');
      $return = TRUE;
      return TRUE;
    }
    elseif (substr($first_int, 0, 4) == 6541 and substr($first_int, -4, 8) >= 0300 and substr($first_int, -4, 8) <= 5099) {
      $return = TRUE;
      return TRUE;
    }
    elseif (substr($first_int, 0, 4) == 6542 and substr($first_int, -4, 8) >= 0300 and substr($first_int, -4, 8) <= 5099) {
      $return = TRUE;
      return TRUE;
    }
    elseif (substr($first_int, 0, 4) == 6543 and substr($first_int, -4, 8) >= 0300 and substr($first_int, -4, 8) <= 2099) {
      $return = TRUE;
      return TRUE;
    }
    elseif (substr($first_int, 0, 4) == 6543 and substr($first_int, -4, 8) >= 2200 and substr($first_int, -4, 8) <= 5099) {
      $return = TRUE;
      return TRUE;
    }
    elseif (substr($first_int, 0, 4) == 6556 and substr($first_int, -4, 8) >= 0300 and substr($first_int, -4, 8) <= 5099) {
      $return = TRUE;
      return TRUE;
    }
    elseif (substr($first_int, 0, 4) == 6558 and substr($first_int, -4, 8) >= 0300 and substr($first_int, -4, 8) <= 5099) {
      $return = TRUE;
      return TRUE;
    }
    elseif (substr($first_int, 0, 4) == 6559 and substr($first_int, -4, 8) >= 0300 and substr($first_int, -4, 8) <= 5099) {
      $return = TRUE;
      return TRUE;
    }
    else {
      $return = 'Invalid card number';
      drupal_set_message(t('Invalid card number'), 'error');
      return FALSE;
    }
  }
  else {
    $return = 'Invalid card number';
    drupal_set_message(t('Invalid card number'), 'error');
    return FALSE;
  }

  //Check the security code.
  $int = intval($card_type['code']);
  if (strlen($card_type['code']) != 3 or !is_int($int) or $int < 0) {
    $return = 'Invalid security code';
    drupal_set_message(t('Invalid security code'), 'error');
    return FALSE;
  }

  //Check the expiration date.
  if ($card_type['exp_year'] < date('y') or $card_type['exp_month'] < date('m')) {
    $return = 'You have entered an expired credit card.';
    drupal_set_message(t('You have entered an expired credit card.'), 'error');
    return FALSE;
  }
  return $return;
}

/**
 * Submit the payment request.
 */
function commerce_ogone_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $charge) {
  $customer_profile = commerce_ogone_customer_profile_load($order->commerce_customer_billing['und'][0]['profile_id']);
  global $base_root;
  $api = commerce_ogone_api_object();
  $result = $api->directPayments($customer_profile, $order, $pane_values);
  // drupal_goto('commerce_ogone/test/test', array( 'result' => $result));
  $data = $api->get_response_data($result->data);

  if ($payment_method['settings']['3d_secure'] == 1 and $data['STATUS'] == '5') {
    //Delete the transaction on Ogone site.

    drupal_goto('commerce_ogone/3ds/callback', array('query' => array(
      'STATUS' => $data['STATUS'],
      'PAYID' => $data['PAYID'],
      'ORDERID' => $order->order_id,
      '3D' => 'failed',
      'REMOTE_ORDER_ID' => $data['orderID'],
      'NCERROR' => $data['NCERROR'],
      'NCSTATUS' => $data['NCSTATUS'],
      'NCERRORPLUS' => $data['NCERRORPLUS'],
      )
    ));
  }

  if (strstr($data['NCERRORPLUS'], 'unknown order/1/i') != FALSE) {
    $error = 'Your IP Address is not set on Ogone site. Go to your account in Ogone and then Technical information->Data and origin verification and set your IP Address.';
    drupal_set_message(t('@error', array('@error' => $error)), 'error');
    drupal_goto($base_root);
  }
  //Check for 3d secure.
  if ($data['STATUS'] == 46) {
    $xml = simplexml_load_string($result->data);
    foreach ($xml as $key => $value) {
      $tags[$key] = (string)$value;
    }

    drupal_goto('commerce_ogone/3d_secure', array('query' => array(
      'result' => $tags['HTML_ANSWER'])
    ));

    return $result;
  }

  if ($result->status_message and $result->code == '200' and $data['NCERROR'] == 0 and ($data['STATUS'] == 5 or $data['STATUS'] == 9) ) {
    $transaction = commerce_payment_transaction_new('ogone_direct', $order->order_id);
    $transaction->instance_id = $payment_method['instance_id'];
    $transaction->remote_id = !empty($data['PAYID']) ? $data['PAYID'] : '';
    $transaction->amount = $order->commerce_order_total['und'][0]['amount'];
    $transaction->currency_code = $order->commerce_order_total['und'][0]['currency_code'];
    $transaction->status = ($payment_method['settings']['transaction_type_process'] == 'capture_manual') ? COMMERCE_PAYMENT_STATUS_PENDING : COMMERCE_PAYMENT_STATUS_OGONE_CAPTURED;
    //$transaction->message = 'Payment requested';
    $transaction->message_variables = array('@name' => $customer_profile[0]->commerce_customer_address['und'][0]['name_line']);
    if ($payment_method['settings']['transaction_type_process'] == 'capture_manual') {
      $transaction->message = 'Payment requested';
    }
    else {
      $transaction->message = 'Payment requested';
    }
    commerce_payment_transaction_save($transaction);
  }
  else {
    $transaction = commerce_payment_transaction_new('ogone_direct', $order->order_id);
    $transaction->instance_id = $payment_method['instance_id'];
    $transaction->amount = $order->commerce_order_total['und'][0]['amount'];
    $transaction->currency_code = $order->commerce_order_total['und'][0]['currency_code'];
    $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
    $transaction->message = $data['NCERRORPLUS'];
    $transaction->message_variables = array('@name' => $customer_profile[0]->commerce_customer_address['und'][0]['name_line']);

    commerce_payment_transaction_save($transaction);
  }
  $payment_methods = commerce_payment_method_instance_load('ogone_onsite|commerce_payment_ogone_onsite');
  $payment_method_account = explode('|', $payment_methods['settings']['account']);

 // drupal_goto('commerce_ogone/test/test', array( 'result' => $result));
  return $result;
}

/**
 * Loads profile by profile_id.
 */
function commerce_ogone_customer_profile_load($profile_id) {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'commerce_customer_profile')
  ->propertyCondition('profile_id', $profile_id)
  ->propertyOrderBy('profile_id', 'DESC');
  $return = array();
  $result = $query->execute();
    if (!empty($result['commerce_customer_profile'])) {
    foreach ($result['commerce_customer_profile'] as $profile) {
      $return[] = entity_load_single('commerce_customer_profile', $profile->profile_id);
    }
  }
  return $return;
}