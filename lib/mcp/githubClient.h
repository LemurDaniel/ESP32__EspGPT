#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <toolTypes.h>

class GithubClient : public Mcp
{
private:
    static constexpr const char *BASE_URL = "https://api.github.com";

    int _status = 0;
    String _error;
    String _token;
    String _username;
    WiFiClientSecure _client;

    void _buildRepoFilter(JsonDocument &filter)
    {
        filter[0]["name"] = true;
        filter[0]["description"] = true;
        filter[0]["language"] = true;
        filter[0]["html_url"] = true;
        filter[0]["stargazers_count"] = true;
        filter[0]["forks_count"] = true;
    }

    JsonDocument _get(const String &path, const JsonDocument *filter = nullptr)
    {
        String url = String(BASE_URL) + path;
        Serial.printf("[github] GET %s\n", url.c_str());

        HTTPClient http;
        if (!http.begin(_client, url))
        {
            _error = "http.begin() failed";
            _status = -1;
            Serial.printf("[github] http.begin() failed for %s\n", url.c_str());
            return JsonDocument();
        }
        http.addHeader("Authorization", "Bearer " + _token);
        http.addHeader("Accept", "application/vnd.github+json");
        http.addHeader("X-GitHub-Api-Version", "2022-11-28");
        http.addHeader("User-Agent", "ESP32-GithubClient");

        _status = http.GET();
        Serial.printf("[github] HTTP status: %d\n", _status);

        String response = http.getString();
        http.end();

        if (_status != 200)
        {
            _error = response;
            Serial.printf("[github] error: %s\n", response.c_str());
            return JsonDocument();
        }

        Serial.printf("[github] response length: %d bytes\n", response.length());

        JsonDocument doc;
        DeserializationError err = filter
                                       ? deserializeJson(doc, response, DeserializationOption::Filter(*filter))
                                       : deserializeJson(doc, response);

        if (err)
        {
            _error = "JSON parse error";
            _status = -1;
            Serial.println("[github] JSON parse error");
            return JsonDocument();
        }

        Serial.printf("[github] parsed OK\n");
        return doc;
    }

public:
    int status() const { return _status; }
    String error() const { return _error; }

    GithubClient()
    {
        registerTool("github_get_repo",
                     "Get metadata for a GitHub repository",
                     R"({"type":"object","properties":{"owner":{"type":"string"},"repo":{"type":"string"}},"required":["owner","repo"]})",
                     [this](JsonDocument d) -> String
                     { JsonDocument r = getRepo(d["owner"], d["repo"]); String s; serializeJson(r, s); return s; });

        registerTool("github_get_contents",
                     "List files or get file content from a GitHub repository path",
                     R"({"type":"object","properties":{"owner":{"type":"string"},"repo":{"type":"string"},"path":{"type":"string","description":"File or directory path, empty for root"}},"required":["owner","repo"]})",
                     [this](JsonDocument d) -> String
                     { JsonDocument r = getContents(d["owner"], d["repo"], d["path"] | ""); String s; serializeJson(r, s); return s; });

        registerTool("github_get_branches",
                     "List branches of a GitHub repository",
                     R"({"type":"object","properties":{"owner":{"type":"string"},"repo":{"type":"string"}},"required":["owner","repo"]})",
                     [this](JsonDocument d) -> String
                     { JsonDocument r = getBranches(d["owner"], d["repo"]); String s; serializeJson(r, s); return s; });

        registerTool("github_get_commits",
                     "List commits of a GitHub repository",
                     R"({"type":"object","properties":{"owner":{"type":"string"},"repo":{"type":"string"},"query":{"type":"string","description":"Optional query string, e.g. per_page=10&sha=main"}},"required":["owner","repo"]})",
                     [this](JsonDocument d) -> String
                     { JsonDocument r = getCommits(d["owner"], d["repo"], d["query"] | ""); String s; serializeJson(r, s); return s; });

        registerTool("github_list_my_repos",
                     "List repositories of the authenticated GitHub user",
                     R"({"type":"object","properties":{}})",
                     [this](JsonDocument) -> String
                     { JsonDocument r = listMyRepos(); String s; serializeJson(r, s); return s; });

        registerTool("github_list_user_repos",
                     "List public repositories of a GitHub user",
                     R"({"type":"object","properties":{"username":{"type":"string"}},"required":["username"]})",
                     [this](JsonDocument d) -> String
                     { JsonDocument r = listUserRepos(d["username"]); String s; serializeJson(r, s); return s; });
    }

    GithubClient &begin(const String &username, const String &token)
    {
        _username = username;
        _token = token;
        _client.setInsecure();
        return *this;
    }

    String username() const { return _username; }

    // GET /repos/{owner}/{repo}
    JsonDocument getRepo(const String &owner, const String &repo)
    {
        return _get("/repos/" + owner + "/" + repo);
    }

    // GET /repos/{owner}/{repo}/contents/{path}
    JsonDocument getContents(const String &owner, const String &repo, const String &path = "")
    {
        return _get("/repos/" + owner + "/" + repo + "/contents/" + path);
    }

    // GET /repos/{owner}/{repo}/branches
    JsonDocument getBranches(const String &owner, const String &repo)
    {
        return _get("/repos/" + owner + "/" + repo + "/branches");
    }

    // GET /repos/{owner}/{repo}/commits  (optional: ?sha=branch&per_page=N)
    JsonDocument getCommits(const String &owner, const String &repo, const String &query = "")
    {
        String path = "/repos/" + owner + "/" + repo + "/commits";
        if (query.length() > 0)
            path += "?" + query;
        return _get(path);
    }

    // GET /user/repos
    JsonDocument listMyRepos()
    {
        JsonDocument filter;
        _buildRepoFilter(filter);
        return _get("/user/repos?per_page=20", &filter);
    }

    // GET /users/{username}/repos
    JsonDocument listUserRepos(const String &username)
    {
        JsonDocument filter;
        _buildRepoFilter(filter);
        return _get("/users/" + username + "/repos?per_page=20", &filter);
    }
};
