package bgu.spl.net.impl.stomp;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import bgu.spl.net.api.MessageEncoderDecoder;

public class FrameMessageEncodeDecoder implements MessageEncoderDecoder<Frame> {

    private byte[] bytes = new byte[1 << 10]; // start with 1k
    private int len = 0;

    @Override
    public Frame decodeNextByte(byte nextByte) {
        if (nextByte == '\u0000') {
            System.out.println(new String(bytes, 0, len, StandardCharsets.UTF_8)+"END");
            return popFrame();
        }
        // System.out.println(new String(bytes, 0, len, StandardCharsets.UTF_8));

        pushByte(nextByte);
        return null; // not a line yet
    }

    @Override
    public byte[] encode(Frame message) {
        String result = message.toString();
        return result.getBytes(); // uses utf8 by default
    }

    private void pushByte(byte nextByte) {
        if (len >= bytes.length) {
            bytes = Arrays.copyOf(bytes, len * 2);
        }

        bytes[len++] = nextByte;
    }

    private Frame popFrame() {
        String str = new String(bytes, 0, len, StandardCharsets.UTF_8);
        StringBuilder strBuild = new StringBuilder(str);
        len = 0;

        String command = "";
        Map<String, String> headers = new HashMap<>();
        String body = "";

        command = getCommand(strBuild);
        headers = getHeaders(strBuild);
        body = getBody(strBuild);

        Frame result = new Frame(command, headers, body);
        return result;
    }

    String getCommand(StringBuilder str) {
        String command = "";
        char c = str.charAt(0);
        while (c != '\n') {
            command += c;
            str = str.deleteCharAt(0);
            c = str.charAt(0);
        }
        str = str.deleteCharAt(0);

        return command;
    }

    Map<String, String> getHeaders(StringBuilder str) {
        Map<String, String> headers = new HashMap<>();
        char c = str.charAt(0);
        while (c != '\n') {
            String key = "";
            String value = "";
            while (c != '\n') {
                if (c == ':') {
                    key = value;
                    value = "";
                } else {
                    value += c;
                }
                str = str.deleteCharAt(0);
                c = str.charAt(0);
            }
            headers.put(key, value);
            str = str.deleteCharAt(0);
            c = str.charAt(0);
        }
        str = str.deleteCharAt(0);
        return headers;
    }

    String getBody(StringBuilder str) {
        String body = "";
        // char c = str.charAt(0);
        while (str.length() > 0) {
            body += str.charAt(0);
            str = str.deleteCharAt(0);
            // c = str.charAt(0);  
        }
        // str = str.deleteCharAt(0);
        return body;
    }

}
