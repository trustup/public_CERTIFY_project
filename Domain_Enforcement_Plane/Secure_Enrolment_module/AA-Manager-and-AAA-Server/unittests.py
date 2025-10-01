import unittest
from unittest.mock import patch, MagicMock
import socket
import threading
import time

from bootstrapping_proxy import connect_to_aaa_server, main as bootstrapping_main
from attestation_proxy import connect_to_att_server

HOST = '127.0.0.1'
PORT = 33000
AAA_SERVER_HOST = '127.0.0.1'
AAA_SERVER_PORT = 33333


class TestConnectToAttServer(unittest.TestCase):

    @patch('socket.socket')
    def test_connect_to_att_server_success(self, mock_socket):
        """Test successful connection and message sending."""
        mock_socket_instance = MagicMock()
        mock_socket.return_value.__enter__.return_value = mock_socket_instance

        test_message = b'\x04\x01\x03ATT'
        expected_response = b'\x08ACK'
        mock_socket_instance.recv.return_value = expected_response

        response = connect_to_att_server(test_message)

        mock_socket_instance.connect.assert_called_once_with(('127.0.0.1', 44444))
        mock_socket_instance.sendall.assert_called_once_with(test_message)
        mock_socket_instance.recv.assert_called_once_with(2048)

        self.assertEqual(response, expected_response)

    @patch('socket.socket')
    def test_connect_to_att_server_failure(self, mock_socket):
        """Test handling of connection failure."""
        print("Running ATT server unknown message format test.")
        mock_socket.return_value.__enter__.side_effect = socket.error("Connection failed")

        test_message = b'\x04\x01\x03ATT'
        response = connect_to_att_server(test_message)

        self.assertIsNone(response)



class TestBootstrappingProxy(unittest.TestCase):

    @patch('socket.socket')
    def test_hello_message_response(self, mock_socket):
        """
        Test that the proxy correctly responds with "Hello" when receiving "Hello" from the board.
        """
        mock_client_socket = MagicMock()
        mock_client_socket.recv.side_effect = [b"Hello", b""]

        mock_socket_instance = MagicMock()
        mock_socket_instance.accept.return_value = (mock_client_socket, ('127.0.0.1', 50000))

        mock_socket.return_value = mock_socket_instance

        server_thread = threading.Thread(target=bootstrapping_main, daemon=True)
        server_thread.start()
        time.sleep(1)

        mock_client_socket.sendall.assert_called_with(b"Hello")

    @patch('socket.socket')
    def test_unknown_message_handling(self, mock_socket):
        """
        Test that the proxy logs an error for an unknown message type.
        """
        test_message = bytes([0xFF, 0x01, 0x02])

        mock_client_socket = MagicMock()
        mock_client_socket.recv.side_effect = [test_message, b""]

        mock_socket_instance = MagicMock()
        mock_socket_instance.accept.return_value = (mock_client_socket, ('127.0.0.1', 50000))
        mock_socket.return_value = mock_socket_instance

        server_thread = threading.Thread(target=bootstrapping_main, daemon=True)
        server_thread.start()
        time.sleep(1)

        mock_client_socket.sendall.assert_not_called()



if __name__ == '__main__':
    unittest.main()