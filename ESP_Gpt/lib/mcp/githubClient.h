#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <tool.h>

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
        Tool tGetRepo("github_get_repo", "Get metadata for a GitHub repository");
        tGetRepo.param("owner").string();
        tGetRepo.param("repo").string();
        tGetRepo.does(std::bind(&GithubClient::getRepo, this, std::placeholders::_1));
        registerTool(tGetRepo);

        Tool tGetContents("github_get_contents", "List files or get file content from a GitHub repository path");
        tGetContents.param("owner").string();
        tGetContents.param("repo").string();
        tGetContents.param("path").description("File or directory path, empty for root").string().optional();
        tGetContents.does(std::bind(&GithubClient::getContents, this, std::placeholders::_1));
        registerTool(tGetContents);

        Tool tGetBranches("github_get_branches", "List branches of a GitHub repository");
        tGetBranches.param("owner").string();
        tGetBranches.param("repo").string();
        tGetBranches.does(std::bind(&GithubClient::getBranches, this, std::placeholders::_1));
        registerTool(tGetBranches);

        Tool tGetCommits("github_get_commits", "List commits of a GitHub repository");
        tGetCommits.param("owner").string();
        tGetCommits.param("repo").string();
        tGetCommits.param("query").description("Optional query string, e.g. per_page=10&sha=main").string().optional();
        tGetCommits.does(std::bind(&GithubClient::getCommits, this, std::placeholders::_1));
        registerTool(tGetCommits);

        Tool tListMyRepos("github_list_my_repos", "List repositories of the authenticated GitHub user");
        tListMyRepos.does(std::bind(&GithubClient::listMyRepos, this, std::placeholders::_1));
        registerTool(tListMyRepos);

        Tool tListUserRepos("github_list_user_repos", "List public repositories of a GitHub user");
        tListUserRepos.param("username").string();
        tListUserRepos.does(std::bind(&GithubClient::listUserRepos, this, std::placeholders::_1));
        registerTool(tListUserRepos);
    }

    GithubClient &begin(const String &username, const String &token)
    {
        _username = username;
        _token = token;
        _client.setInsecure();
        return *this;
    }

    String username() const { return _username; }

    String getRepo(JsonDocument d)
    {
        JsonDocument r = _get("/repos/" + d["owner"].as<String>() + "/" + d["repo"].as<String>());
        String s; serializeJson(r, s); return s;
    }

    String getContents(JsonDocument d)
    {
        String path = "/repos/" + d["owner"].as<String>() + "/" + d["repo"].as<String>() + "/contents/" + d["path"].as<String>();
        JsonDocument r = _get(path);
        String s; serializeJson(r, s); return s;
    }

    String getBranches(JsonDocument d)
    {
        JsonDocument r = _get("/repos/" + d["owner"].as<String>() + "/" + d["repo"].as<String>() + "/branches");
        String s; serializeJson(r, s); return s;
    }

    String getCommits(JsonDocument d)
    {
        String path = "/repos/" + d["owner"].as<String>() + "/" + d["repo"].as<String>() + "/commits";
        String query = d["query"] | "";
        if (query.length() > 0)
            path += "?" + query;
        JsonDocument r = _get(path);
        String s; serializeJson(r, s); return s;
    }

    String listMyRepos(JsonDocument)
    {
        JsonDocument filter;
        _buildRepoFilter(filter);
        JsonDocument r = _get("/user/repos?per_page=20", &filter);
        String s; serializeJson(r, s); return s;
    }

    String listUserRepos(JsonDocument d)
    {
        JsonDocument filter;
        _buildRepoFilter(filter);
        JsonDocument r = _get("/users/" + d["username"].as<String>() + "/repos?per_page=20", &filter);
        String s; serializeJson(r, s); return s;
    }
};
